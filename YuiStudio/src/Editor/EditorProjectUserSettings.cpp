#include "pch.h"

#include "EditorProjectUserSettings.h"

#include "Yuicy/Core/Log.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>
#include <system_error>

namespace Yuicy {

	static std::filesystem::path GetUserSettingsDirectory(const std::filesystem::path& projectDirectory)
	{
		return projectDirectory / ".yuistudio";
	}

	static void EnsureDirectoryIgnoredByGit(const std::filesystem::path& directory)
	{
		const std::filesystem::path gitignorePath = directory / ".gitignore";
		std::error_code ec;
		if (std::filesystem::exists(gitignorePath, ec))
			return;

		std::ofstream fout(gitignorePath);
		fout << "# YuiStudio personal editor settings, do not commit\n*\n";
		if (!fout)
			YUICY_WARN("[EditorUserSettings] Failed to write {}", gitignorePath.string());
	}

	std::filesystem::path EditorProjectUserSettingsSerializer::GetFilePath(const std::filesystem::path& projectDirectory)
	{
		return GetUserSettingsDirectory(projectDirectory) / "EditorUserSettings.yaml";
	}

	bool EditorProjectUserSettingsSerializer::Serialize(const EditorProjectUserSettings& settings, const std::filesystem::path& projectDirectory)
	{
		const std::filesystem::path directory = GetUserSettingsDirectory(projectDirectory);
		std::error_code ec;
		std::filesystem::create_directories(directory, ec);
		if (ec)
		{
			YUICY_ERROR("[EditorUserSettings] Failed to create directory '{}': {}", directory.string(), ec.message());
			return false;
		}

		EnsureDirectoryIgnoredByGit(directory);

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "AutoSave" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Enabled" << YAML::Value << settings.autoSave.enabled;
		out << YAML::Key << "IntervalSeconds" << YAML::Value << settings.autoSave.intervalSeconds;
		out << YAML::EndMap;
		out << YAML::EndMap;

		const std::filesystem::path filepath = GetFilePath(projectDirectory);
		std::ofstream fout(filepath);
		fout << out.c_str();
		if (!fout)
		{
			YUICY_ERROR("[EditorUserSettings] Failed to write {}", filepath.string());
			return false;
		}

		return true;
	}

	std::optional<EditorProjectUserSettings> EditorProjectUserSettingsSerializer::Deserialize(const std::filesystem::path& projectDirectory)
	{
		const std::filesystem::path filepath = GetFilePath(projectDirectory);
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			YUICY_ERROR("[EditorUserSettings] Failed to open {}", filepath.string());
			return std::nullopt;
		}

		std::stringstream content;
		content << stream.rdbuf();

		try
		{
			const YAML::Node data = YAML::Load(content.str());
			if (!data.IsMap())
			{
				YUICY_ERROR("[EditorUserSettings] Invalid file, root must be a map: {}", filepath.string());
				return std::nullopt;
			}

			EditorProjectUserSettings settings;
			if (const YAML::Node autoSaveNode = data["AutoSave"])
			{
				if (const YAML::Node enabledNode = autoSaveNode["Enabled"])
					settings.autoSave.enabled = enabledNode.as<bool>();
				if (const YAML::Node intervalNode = autoSaveNode["IntervalSeconds"])
					settings.autoSave.intervalSeconds = intervalNode.as<int>();
			}

			return settings;
		}
		catch (const YAML::Exception& e)
		{
			YUICY_ERROR("[EditorUserSettings] Failed to parse '{}': {}", filepath.string(), e.what());
			return std::nullopt;
		}
	}

}
