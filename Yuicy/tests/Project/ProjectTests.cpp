#include "pch.h"

#include "Yuicy/Project/Project.h"

using namespace Yuicy;

TEST_SUITE("Project")
{
	TEST_CASE("Project derives its directories from the project file")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectDirectory = tempDirectory.GetPath() / "Game";

		ProjectSettings settings;
		settings.AssetDirectory = "Content";
		const Project project(projectDirectory / "Sub" / ".." / "Game.yproj", settings);

		CHECK(project.GetProjectFile() == (projectDirectory / "Game.yproj").lexically_normal());
		CHECK(project.GetProjectDirectory() == projectDirectory.lexically_normal());
		CHECK(project.GetAssetDirectory() == projectDirectory.lexically_normal() / "Content");
		CHECK(project.GetAssetRegistryPath() == projectDirectory.lexically_normal() / "Content" / "AssetRegistry.yregistry");
	}

	TEST_CASE("Project resolves a relative project file against the working directory")
	{
		const Project project("Game/Game.yproj", ProjectSettings{});

		CHECK(project.GetProjectFile().is_absolute());
		CHECK(project.GetProjectFile() == (std::filesystem::current_path() / "Game" / "Game.yproj").lexically_normal());
	}
}
