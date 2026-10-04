#include "pch.h"

#include "EditorProjectSession.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Project/BuiltinAssetLoaders.h"
#include "Yuicy/Project/ProjectSceneContext.h"
#include "Yuicy/Project/ProjectSerializer.h"

#include <system_error>

namespace Yuicy {

	static bool EnsureDirectoryExists(const std::filesystem::path& directory)
	{
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		if (ec)
		{
			YUICY_ERROR("[Project] Failed to create directory '{}': {}", directory.string(), ec.message());
			return false;
		}

		return true;
	}

	Scope<EditorProjectSession> EditorProjectSession::Create(const std::filesystem::path& projectFile)
	{
		const std::filesystem::path normalizedFile = projectFile.lexically_normal();

		Ref<Project> project = CreateRef<Project>();
		ProjectConfig& config = project->GetConfig();
		config.Name = normalizedFile.stem().string();
		config.ProjectDirectory = normalizedFile.parent_path().string();
		config.ProjectFileName = normalizedFile.filename().string();

		if (!EnsureDirectoryExists(project->GetAssetDirectory()))
			return nullptr;
		if (!EnsureDirectoryExists(std::filesystem::path(config.ProjectDirectory) / config.ScriptDirectory))
			return nullptr;
		if (!ProjectSerializer(project).Serialize(normalizedFile))
			return nullptr;

		return CreateScope<EditorProjectSession>(ConstructionKey{}, normalizedFile, std::move(project));
	}

	Scope<EditorProjectSession> EditorProjectSession::Open(const std::filesystem::path& projectFile)
	{
		const std::filesystem::path normalizedFile = projectFile.lexically_normal();

		Ref<Project> project = CreateRef<Project>();
		if (!ProjectSerializer(project).Deserialize(normalizedFile))
			return nullptr;

		std::error_code ec;
		if (!std::filesystem::is_directory(project->GetAssetDirectory(), ec))
		{
			YUICY_ERROR("[Project] Asset directory not found: {}", project->GetAssetDirectory().string());
			return nullptr;
		}

		return CreateScope<EditorProjectSession>(ConstructionKey{}, normalizedFile, std::move(project));
	}

	EditorProjectSession::EditorProjectSession(ConstructionKey, std::filesystem::path projectFile, Ref<Project> project)
		: m_projectFile(std::move(projectFile)),
		  m_project(std::move(project)),
		  m_assetManager(CreateRef<EditorAssetManager>(EditorAssetManagerSpecification{
			  m_project->GetAssetDirectory(), m_project->GetAssetRegistryPath(), CreateBuiltinAssetLoaders() }))
	{
	}

	EditorProjectSession::~EditorProjectSession() = default;

	SceneContext EditorProjectSession::MakeSceneContext() const
	{
		return Yuicy::MakeSceneContext(m_project->GetConfig(), m_assetManager);
	}

	bool EditorProjectSession::Save()
	{
		return ProjectSerializer(m_project).Serialize(m_projectFile);
	}

}
