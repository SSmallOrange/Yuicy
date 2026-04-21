#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Assert.h"
#include "Yuicy/Renderer/SortingLayerConfig.h"
#include "Yuicy/Physics/CollisionLayerConfig.h"

#include <filesystem>
#include <string>

namespace Yuicy {

	class EditorAssetManager;

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

		// 活动项目管理
		static Ref<Project> GetActive() { return s_activeProject; }
		static void SetActive(const Ref<Project>& project);

		// 资源管理
		static Ref<EditorAssetManager> GetEditorAssetManager() { return s_assetManager; }

		// 路径工具方法
		static const std::string& GetProjectName()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return s_activeProject->GetConfig().Name;
		}

		static std::filesystem::path GetProjectDirectory()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return s_activeProject->GetConfig().ProjectDirectory;
		}

		std::filesystem::path GetAssetDirectory() const
		{
			return std::filesystem::path(m_config.ProjectDirectory) / m_config.AssetDirectory;
		}

		static std::filesystem::path GetActiveAssetDirectory()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return s_activeProject->GetAssetDirectory();
		}

		static std::filesystem::path GetScriptDirectory()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return std::filesystem::path(s_activeProject->GetConfig().ProjectDirectory)
				/ s_activeProject->GetConfig().ScriptDirectory;
		}

		static std::filesystem::path GetCacheDirectory()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return std::filesystem::path(s_activeProject->GetConfig().ProjectDirectory) / "Cache";
		}

		static std::filesystem::path GetAssetRegistryPath()
		{
			YUICY_CORE_ASSERT(s_activeProject);
			return s_activeProject->GetAssetDirectory() / "AssetRegistry.yregistry";
		}

	private:
		ProjectConfig m_config;

		inline static Ref<Project> s_activeProject;
		inline static Ref<EditorAssetManager> s_assetManager;

		friend class ProjectSerializer;
	};

}
