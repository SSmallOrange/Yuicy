#include "pch.h"

#include "EditorProjectSession.h"
#include "Asset/EditorAssetManager.h"

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

	static EditorProjectUserSettings LoadOrCreateUserSettings(const std::filesystem::path& projectDirectory)
	{
		std::error_code ec;
		if (!std::filesystem::exists(EditorProjectUserSettingsSerializer::GetFilePath(projectDirectory), ec))
		{
			EditorProjectUserSettings defaults;
			EditorProjectUserSettingsSerializer::Serialize(defaults, projectDirectory);
			return defaults;
		}

		if (std::optional<EditorProjectUserSettings> settings = EditorProjectUserSettingsSerializer::Deserialize(projectDirectory))
			return *settings;

		YUICY_WARN("[Project] Using default editor user settings");
		return {};
	}

	Scope<EditorProjectSession> EditorProjectSession::Create(const std::filesystem::path& projectFile)
	{
		ProjectSettings settings;
		settings.Name = projectFile.stem().string();
		Project project(projectFile, std::move(settings));

		if (!EnsureDirectoryExists(project.GetAssetDirectory()))
			return nullptr;
		if (!EnsureDirectoryExists(project.GetAssetDirectory() / "Scripts"))
			return nullptr;
		if (!ProjectSerializer::Serialize(project.GetSettings(), project.GetProjectFile()))
			return nullptr;

		EditorProjectUserSettings userSettings;
		EditorProjectUserSettingsSerializer::Serialize(userSettings, project.GetProjectDirectory());

		return CreateScope<EditorProjectSession>(ConstructionKey{}, std::move(project), userSettings);
	}

	Scope<EditorProjectSession> EditorProjectSession::Open(const std::filesystem::path& projectFile)
	{
		std::optional<ProjectSettings> settings = ProjectSerializer::Deserialize(projectFile);
		if (!settings)
			return nullptr;

		Project project(projectFile, std::move(*settings));

		std::error_code ec;
		if (!std::filesystem::is_directory(project.GetAssetDirectory(), ec))
		{
			YUICY_ERROR("[Project] Asset directory not found: {}", project.GetAssetDirectory().string());
			return nullptr;
		}

		EditorProjectUserSettings userSettings = LoadOrCreateUserSettings(project.GetProjectDirectory());
		return CreateScope<EditorProjectSession>(ConstructionKey{}, std::move(project), userSettings);
	}

	EditorProjectSession::EditorProjectSession(ConstructionKey, Project project, EditorProjectUserSettings userSettings)
		: m_project(std::move(project)),
		  m_userSettings(userSettings),
		  m_assetManager(CreateRef<EditorAssetManager>(EditorAssetManagerSpecification{
			  m_project.GetAssetDirectory(), m_project.GetAssetRegistryPath(), CreateBuiltinAssetLoaders() }))
	{
	}

	EditorProjectSession::~EditorProjectSession() = default;

	SceneContext EditorProjectSession::MakeSceneContext() const
	{
		return Yuicy::MakeSceneContext(m_project.GetSettings(), m_assetManager);
	}

	bool EditorProjectSession::Save()
	{
		return ProjectSerializer::Serialize(m_project.GetSettings(), m_project.GetProjectFile());
	}

}
