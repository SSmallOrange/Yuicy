#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Scene/SceneContext.h"

#include "EditorProjectUserSettings.h"

#include <filesystem>

namespace Yuicy {

	class EditorAssetManager;

	// 析构时资源管理器把资产注册表写到 Project::GetAssetRegistryPath()
	class EditorProjectSession
	{
		struct ConstructionKey
		{
			explicit ConstructionKey() = default;
		};

	public:
		// 创建资产目录与 Scripts 目录，写出 .yproj 与个人设置文件；目录创建或 .yproj 写入失败时返回 nullptr
		static Scope<EditorProjectSession> Create(const std::filesystem::path& projectFile);
		// .yproj 无法读取或资产目录不存在时返回 nullptr；个人设置文件缺失时写出默认值，无法解析时使用默认值
		static Scope<EditorProjectSession> Open(const std::filesystem::path& projectFile);

		// 只能经由 Create / Open 构造：资产目录不存在时 EditorAssetManager 扫描目录会抛出异常
		EditorProjectSession(ConstructionKey, Project project, EditorProjectUserSettings userSettings);
		~EditorProjectSession();

		EditorProjectSession(const EditorProjectSession&) = delete;
		EditorProjectSession& operator=(const EditorProjectSession&) = delete;

		Project& GetProject() { return m_project; }
		const Project& GetProject() const { return m_project; }
		const Ref<EditorAssetManager>& GetAssetManager() const { return m_assetManager; }
		EditorProjectUserSettings& GetUserSettings() { return m_userSettings; }
		const EditorProjectUserSettings& GetUserSettings() const { return m_userSettings; }

		SceneContext MakeSceneContext() const;

		// 只写 .yproj，失败时返回 false
		bool Save();

	private:
		Project m_project;
		EditorProjectUserSettings m_userSettings;
		Ref<EditorAssetManager> m_assetManager;
	};

}
