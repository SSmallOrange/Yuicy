#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Project/Project.h"

#include <filesystem>
#include <string>

namespace Yuicy::Test {

	// 以 projectDirectory 为项目目录创建内存 Project 并设为活动项目，析构时关闭项目（不恢复之前的活动项目，不支持嵌套）。
	// Scene、SceneSerializer、AssetManager 都假定存在活动项目。
	// projectDirectory 必须比本对象活得久：关闭项目时 EditorAssetManager 会把资产注册表写回 <projectDirectory>/Assets/。
	class ScopedActiveProject
	{
	public:
		static constexpr const char* kAssetDirectoryName = "Assets";

		explicit ScopedActiveProject(const std::filesystem::path& projectDirectory, const std::string& name = "YuicyTestProject");
		~ScopedActiveProject();

		ScopedActiveProject(const ScopedActiveProject&) = delete;
		ScopedActiveProject& operator=(const ScopedActiveProject&) = delete;

		const Ref<Project>& GetProject() const { return m_Project; }
		std::filesystem::path GetAssetDirectory() const { return m_Project->GetAssetDirectory(); }

	private:
		Ref<Project> m_Project;
	};

}
