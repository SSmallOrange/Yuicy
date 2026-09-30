#include "YuicyTest/ScopedProject.h"

#include <doctest/doctest.h>

namespace Yuicy::Test {

	ScopedActiveProject::ScopedActiveProject(const std::filesystem::path& projectDirectory, const std::string& name)
	{
		REQUIRE_MESSAGE(!Project::GetActive(), "ScopedActiveProject does not support nesting");

		m_Project = CreateRef<Project>();
		auto& config = m_Project->GetConfig();
		config.Name = name;
		config.ProjectDirectory = projectDirectory.string();
		config.AssetDirectory = kAssetDirectoryName;

		// SetActive 会立即创建 EditorAssetManager 并遍历资产目录，目录不存在时 directory_iterator 会抛异常
		std::filesystem::create_directories(m_Project->GetAssetDirectory());
		Project::SetActive(m_Project);
	}

	ScopedActiveProject::~ScopedActiveProject()
	{
		Project::SetActive(nullptr);
	}

}
