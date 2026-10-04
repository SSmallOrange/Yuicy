#pragma once

#include "Yuicy/Renderer/SortingLayerConfig.h"
#include "Yuicy/Physics/CollisionLayerConfig.h"

#include <filesystem>
#include <string>

namespace Yuicy {

	struct ProjectConfig
	{
		std::string Name = "Untitled";

		// 资产目录
		std::string AssetDirectory = "Assets";

		// Lua 脚本目录
		std::string ScriptDirectory = "Assets/Scripts";

		// 起始场景路径
		std::string StartScene;

		// 自动保存
		bool EnableAutoSave = false;
		int AutoSaveIntervalSeconds = 300;

		std::string ProjectFileName;
		std::string ProjectDirectory;

		// Sorting Layer 配置
		SortingLayerConfig SortingLayers;

		// Collision Layer 配置
		CollisionLayerConfig CollisionLayers;
	};

	class Project
	{
	public:
		Project() = default;
		~Project() = default;

		const ProjectConfig& GetConfig() const { return m_config; }
		ProjectConfig& GetConfig() { return m_config; }

		std::filesystem::path GetAssetDirectory() const
		{
			return std::filesystem::path(m_config.ProjectDirectory) / m_config.AssetDirectory;
		}

		std::filesystem::path GetAssetRegistryPath() const
		{
			return GetAssetDirectory() / "AssetRegistry.yregistry";
		}

	private:
		ProjectConfig m_config;

		friend class ProjectSerializer;
	};

}
