#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Scene/SceneContext.h"

#include <filesystem>

namespace Yuicy {

	class EditorAssetManager;

	// 编辑器中已打开的项目：持有 Project 与该项目的资源管理器；析构时资源管理器把注册表写回本项目的资产目录
	class EditorProjectSession
	{
		struct ConstructionKey
		{
			explicit ConstructionKey() = default;
		};

	public:
		// 创建资产目录与脚本目录并写出 .yproj；失败时返回 nullptr
		static Scope<EditorProjectSession> Create(const std::filesystem::path& projectFile);
		// .yproj 无法读取或资产目录不存在时返回 nullptr
		static Scope<EditorProjectSession> Open(const std::filesystem::path& projectFile);

		// 只能经由 Create / Open 构造：资产目录不存在时 EditorAssetManager 扫描目录会抛出异常
		EditorProjectSession(ConstructionKey, std::filesystem::path projectFile, Ref<Project> project);
		~EditorProjectSession();

		EditorProjectSession(const EditorProjectSession&) = delete;
		EditorProjectSession& operator=(const EditorProjectSession&) = delete;

		Project& GetProject() { return *m_project; }
		const Project& GetProject() const { return *m_project; }
		const std::filesystem::path& GetProjectFile() const { return m_projectFile; }
		const Ref<EditorAssetManager>& GetAssetManager() const { return m_assetManager; }

		SceneContext MakeSceneContext() const;

		// 把配置写回 .yproj，失败时返回 false；资产注册表不在这里写出
		bool Save();

	private:
		std::filesystem::path m_projectFile;
		Ref<Project> m_project;
		Ref<EditorAssetManager> m_assetManager;
	};

}
