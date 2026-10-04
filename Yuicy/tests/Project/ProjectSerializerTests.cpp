#include "pch.h"

#include "Yuicy/Project/ProjectSerializer.h"

using namespace Yuicy;

TEST_SUITE("Project")
{
	TEST_CASE("ProjectSerializer round-trip preserves every section")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectPath = tempDirectory.GetPath() / "Game.yproj";

		ProjectSettings source;
		source.Name = "Game";
		source.AssetDirectory = "Content";
		source.StartScene = std::filesystem::path(u8"Scenes/关卡 1.yui");
		source.Renderer2D.SortingLayers.AddLayer("Particles", 150);
		source.Renderer2D.SortingLayers.RemoveLayer("UI");
		source.Physics2D.CollisionLayers.LayerNames[3] = "Enemy";
		source.Physics2D.CollisionLayers.LayerNames[15] = "Trigger";

		REQUIRE(ProjectSerializer::Serialize(source, projectPath));

		const std::optional<ProjectSettings> loaded = ProjectSerializer::Deserialize(projectPath);
		REQUIRE(loaded);

		CHECK(loaded->Name == "Game");
		CHECK(loaded->AssetDirectory == "Content");
		CHECK(loaded->StartScene == source.StartScene);

		const auto& layers = loaded->Renderer2D.SortingLayers.Layers;
		REQUIRE(layers.size() == source.Renderer2D.SortingLayers.Layers.size());
		for (size_t i = 0; i < layers.size(); i++)
		{
			CAPTURE(i);
			CHECK(layers[i].Name == source.Renderer2D.SortingLayers.Layers[i].Name);
			CHECK(layers[i].Order == source.Renderer2D.SortingLayers.Layers[i].Order);
		}
		CHECK_FALSE(loaded->Renderer2D.SortingLayers.HasLayer("UI"));

		CHECK(loaded->Physics2D.CollisionLayers.LayerNames == source.Physics2D.CollisionLayers.LayerNames);
	}

	TEST_CASE("ProjectSerializer reads module settings from their own sections")
	{
		Test::ScopedTempDirectory tempDirectory;
		const auto path = tempDirectory.WriteFile("Game.yproj",
			"Project:\n"
			"  Name: MyGame\n"
			"  AssetDirectory: Assets\n"
			"  StartScene: Scenes/StartScene.yui\n"
			"Renderer2D:\n"
			"  SortingLayers:\n"
			"    - { Name: Background, Order: -100 }\n"
			"    - { Name: Default, Order: 0 }\n"
			"    - { Name: Water, Order: 50 }\n"
			"Physics2D:\n"
			"  CollisionLayers: [Default, Player, Enemy]\n");

		const std::optional<ProjectSettings> settings = ProjectSerializer::Deserialize(path);
		REQUIRE(settings);

		CHECK(settings->Name == "MyGame");
		CHECK(settings->AssetDirectory == "Assets");
		CHECK(settings->StartScene == "Scenes/StartScene.yui");

		REQUIRE(settings->Renderer2D.SortingLayers.Layers.size() == 3);
		CHECK(settings->Renderer2D.SortingLayers.GetLayerOrder("Water") == 50);
		CHECK_FALSE(settings->Renderer2D.SortingLayers.HasLayer("UI"));

		const auto& layerNames = settings->Physics2D.CollisionLayers.LayerNames;
		CHECK(layerNames[1] == "Player");
		CHECK(layerNames[2] == "Enemy");
		CHECK(layerNames[3] == CollisionLayerConfig().LayerNames[3]);
	}

	TEST_CASE("ProjectSerializer treats empty path values as unset")
	{
		Test::ScopedTempDirectory tempDirectory;
		const auto path = tempDirectory.WriteFile("Game.yproj", "Project:\n  Name: Game\n  AssetDirectory:\n  StartScene:\n");

		const std::optional<ProjectSettings> settings = ProjectSerializer::Deserialize(path);
		REQUIRE(settings);
		CHECK(settings->AssetDirectory == ProjectSettings().AssetDirectory);
		CHECK(settings->StartScene.empty());
	}

	TEST_CASE("ProjectSerializer rejects invalid project files")
	{
		Test::ScopedTempDirectory tempDirectory;

		SUBCASE("missing file")
		{
			CHECK_FALSE(ProjectSerializer::Deserialize(tempDirectory.GetPath() / "Missing.yproj"));
		}

		SUBCASE("malformed YAML")
		{
			const auto path = tempDirectory.WriteFile("Broken.yproj", "Project: [unclosed\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}

		SUBCASE("missing Project section")
		{
			const auto path = tempDirectory.WriteFile("NoRoot.yproj", "Scene: Foo\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}

		SUBCASE("missing project name")
		{
			const auto path = tempDirectory.WriteFile("NoName.yproj", "Project:\n  AssetDirectory: Assets\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}

		SUBCASE("sorting layers are not a sequence")
		{
			const auto path = tempDirectory.WriteFile("BadLayers.yproj", "Project:\n  Name: Game\nRenderer2D:\n  SortingLayers: Default\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}

		SUBCASE("sorting layer without order")
		{
			const auto path = tempDirectory.WriteFile("NoOrder.yproj", "Project:\n  Name: Game\nRenderer2D:\n  SortingLayers:\n    - { Name: Default }\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}

		SUBCASE("collision layers are not a sequence")
		{
			const auto path = tempDirectory.WriteFile("BadCollision.yproj", "Project:\n  Name: Game\nPhysics2D:\n  CollisionLayers: { Default: 0 }\n");
			CHECK_FALSE(ProjectSerializer::Deserialize(path));
		}
	}
}
