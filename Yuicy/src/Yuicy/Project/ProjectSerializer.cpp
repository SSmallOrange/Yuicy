#include "pch.h"
#include "Yuicy/Project/ProjectSerializer.h"

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <fstream>

namespace Yuicy {

	ProjectSerializer::ProjectSerializer(const Ref<Project>& project)
		: m_project(project)
	{
	}

	void ProjectSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Project" << YAML::Value;
		{
			out << YAML::BeginMap;

			out << YAML::Key << "Name" << YAML::Value << m_project->m_config.Name;
			out << YAML::Key << "AssetDirectory" << YAML::Value << m_project->m_config.AssetDirectory;
			out << YAML::Key << "ScriptDirectory" << YAML::Value << m_project->m_config.ScriptDirectory;
			out << YAML::Key << "StartScene" << YAML::Value << m_project->m_config.StartScene;
			out << YAML::Key << "AutoSave" << YAML::Value << m_project->m_config.EnableAutoSave;
			out << YAML::Key << "AutoSaveInterval" << YAML::Value << m_project->m_config.AutoSaveIntervalSeconds;

			out << YAML::EndMap;
		}
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			YUICY_CORE_ERROR("[Project] Failed to save project file: {}", filepath.string());
			return;
		}

		fout << out.c_str();
		YUICY_CORE_INFO("[Project] Saved project: {}", filepath.string());
	}

	bool ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			YUICY_CORE_ERROR("[Project] Failed to open project file: {}", filepath.string());
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data;
		try
		{
			data = YAML::Load(strStream.str());
		}
		catch (const YAML::Exception& e)
		{
			YUICY_CORE_ERROR("[Project] Failed to parse project file '{}': {}", filepath.string(), e.what());
			return false;
		}

		if (!data["Project"])
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, missing 'Project' root node: {}", filepath.string());
			return false;
		}

		YAML::Node rootNode = data["Project"];
		if (!rootNode["Name"])
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, missing project name: {}", filepath.string());
			return false;
		}

		auto& config = m_project->m_config;
		config.Name = rootNode["Name"].as<std::string>();
		config.AssetDirectory = rootNode["AssetDirectory"].as<std::string>(config.AssetDirectory);
		config.ScriptDirectory = rootNode["ScriptDirectory"].as<std::string>(config.ScriptDirectory);
		config.StartScene = rootNode["StartScene"].as<std::string>("");
		config.EnableAutoSave = rootNode["AutoSave"].as<bool>(false);
		config.AutoSaveIntervalSeconds = rootNode["AutoSaveInterval"].as<int>(300);

		std::filesystem::path projectPath = filepath.lexically_normal();
		config.ProjectFileName = projectPath.filename().string();
		config.ProjectDirectory = projectPath.parent_path().string();

		YUICY_CORE_INFO("[Project] Loaded project '{}' from: {}", config.Name, filepath.string());
		return true;
	}

}
