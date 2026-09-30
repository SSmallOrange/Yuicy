#include "pch.h"

#include "Yuicy/Project/Project.h"
#include "Yuicy/Project/ProjectSerializer.h"

using namespace Yuicy;

TEST_SUITE("Project")
{
	TEST_CASE("ProjectSerializer round-trip preserves config and layers")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectPath = tempDirectory.GetPath() / "Game.yproj";

		auto source = CreateRef<Project>();
		auto& config = source->GetConfig();
		config.Name = "Game";
		config.AssetDirectory = "Content";
		config.ScriptDirectory = "Content/Lua";
		config.StartScene = "Scenes/Main.yui";
		config.EnableAutoSave = true;
		config.AutoSaveIntervalSeconds = 42;
		config.SortingLayers.AddLayer("Particles", 150);
		config.SortingLayers.RemoveLayer("UI");
		config.CollisionLayers.LayerNames[3] = "Enemy";
		config.CollisionLayers.LayerNames[15] = "Trigger";

		ProjectSerializer(source).Serialize(projectPath);

		auto loaded = CreateRef<Project>();
		REQUIRE(ProjectSerializer(loaded).Deserialize(projectPath));
		const auto& loadedConfig = loaded->GetConfig();

		CHECK(loadedConfig.Name == "Game");
		CHECK(loadedConfig.AssetDirectory == "Content");
		CHECK(loadedConfig.ScriptDirectory == "Content/Lua");
		CHECK(loadedConfig.StartScene == "Scenes/Main.yui");
		CHECK(loadedConfig.EnableAutoSave);
		CHECK(loadedConfig.AutoSaveIntervalSeconds == 42);

		// 项目目录与文件名来自读取路径而不是文件内容，项目整体搬家后仍能打开
		CHECK(loadedConfig.ProjectFileName == "Game.yproj");
		CHECK(std::filesystem::path(loadedConfig.ProjectDirectory) == projectPath.lexically_normal().parent_path());

		const auto& layers = loadedConfig.SortingLayers.Layers;
		REQUIRE(layers.size() == config.SortingLayers.Layers.size());
		for (size_t i = 0; i < layers.size(); i++)
		{
			CAPTURE(i);
			CHECK(layers[i].Name == config.SortingLayers.Layers[i].Name);
			CHECK(layers[i].Order == config.SortingLayers.Layers[i].Order);
		}
		CHECK_FALSE(loadedConfig.SortingLayers.HasLayer("UI"));

		CHECK(loadedConfig.CollisionLayers.LayerNames == config.CollisionLayers.LayerNames);
	}

	TEST_CASE("ProjectSerializer rejects invalid project files")
	{
		Test::ScopedTempDirectory tempDirectory;
		auto project = CreateRef<Project>();

		SUBCASE("missing file")
		{
			CHECK_FALSE(ProjectSerializer(project).Deserialize(tempDirectory.GetPath() / "Missing.yproj"));
		}

		SUBCASE("malformed YAML")
		{
			const auto path = tempDirectory.WriteFile("Broken.yproj", "Project: [unclosed\n");
			CHECK_FALSE(ProjectSerializer(project).Deserialize(path));
		}

		SUBCASE("missing Project root node")
		{
			const auto path = tempDirectory.WriteFile("NoRoot.yproj", "Scene: Foo\n");
			CHECK_FALSE(ProjectSerializer(project).Deserialize(path));
		}

		SUBCASE("missing project name")
		{
			const auto path = tempDirectory.WriteFile("NoName.yproj", "Project:\n  AssetDirectory: Assets\n");
			CHECK_FALSE(ProjectSerializer(project).Deserialize(path));
		}
	}

	TEST_CASE("ProjectSerializer fills defaults for keys missing from older project files")
	{
		Test::ScopedTempDirectory tempDirectory;
		const auto path = tempDirectory.WriteFile("Old.yproj", "Project:\n  Name: Old\n");

		auto project = CreateRef<Project>();
		REQUIRE(ProjectSerializer(project).Deserialize(path));

		const ProjectConfig defaults;
		const auto& config = project->GetConfig();
		CHECK(config.Name == "Old");
		CHECK(config.AssetDirectory == defaults.AssetDirectory);
		CHECK(config.ScriptDirectory == defaults.ScriptDirectory);
		CHECK_FALSE(config.EnableAutoSave);
		CHECK(config.SortingLayers.Layers.size() == defaults.SortingLayers.Layers.size());
		CHECK(config.CollisionLayers.LayerNames == defaults.CollisionLayers.LayerNames);
	}
}
