#pragma once

#include "Yuicy/Project/ProjectSettings.h"

#include <filesystem>

namespace Yuicy {

	class Project
	{
	public:
		// projectFile 为相对路径时按当前工作目录转为绝对路径
		Project(const std::filesystem::path& projectFile, ProjectSettings settings);

		const std::filesystem::path& GetProjectFile() const { return m_projectFile; }
		std::filesystem::path GetProjectDirectory() const { return m_projectFile.parent_path(); }
		std::filesystem::path GetAssetDirectory() const { return GetProjectDirectory() / m_settings.AssetDirectory; }
		std::filesystem::path GetAssetRegistryPath() const { return GetAssetDirectory() / "AssetRegistry.yregistry"; }

		ProjectSettings& GetSettings() { return m_settings; }
		const ProjectSettings& GetSettings() const { return m_settings; }

	private:
		std::filesystem::path m_projectFile;  // 绝对路径，已规范化
		ProjectSettings m_settings;
	};

}
