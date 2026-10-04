#include "pch.h"

#include "Editor/EditorProjectSession.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"

#include <fstream>
#include <sstream>

using namespace Yuicy;

namespace {

	std::string ReadTextFile(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream content;
		content << stream.rdbuf();
		return content.str();
	}

}

TEST_SUITE("Editor")
{
	TEST_CASE("EditorProjectSession keeps the config after Create then Save then Open")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectFile = tempDirectory.GetPath() / "Game.yproj";

		{
			Scope<EditorProjectSession> session = EditorProjectSession::Create(projectFile);
			REQUIRE(session);
			CHECK(std::filesystem::is_regular_file(projectFile));
			CHECK(std::filesystem::is_directory(session->GetProject().GetAssetDirectory()));
			CHECK(session->GetProject().GetConfig().Name == "Game");

			ProjectConfig& config = session->GetProject().GetConfig();
			config.StartScene = "Scenes/Main.yui";
			config.SortingLayers.AddLayer("Particles", 150);
			config.CollisionLayers.LayerNames[3] = "Enemy";
			REQUIRE(session->Save());
		}

		Scope<EditorProjectSession> reopened = EditorProjectSession::Open(projectFile);
		REQUIRE(reopened);
		CHECK(reopened->GetProjectFile() == projectFile.lexically_normal());

		const ProjectConfig& config = reopened->GetProject().GetConfig();
		CHECK(config.Name == "Game");
		CHECK(config.StartScene == "Scenes/Main.yui");
		CHECK(config.SortingLayers.GetLayerOrder("Particles") == 150);
		CHECK(config.CollisionLayers.LayerNames[3] == "Enemy");

		REQUIRE(reopened->GetAssetManager());
		CHECK(reopened->GetAssetManager()->GetAssetDirectory() == reopened->GetProject().GetAssetDirectory());

		const SceneContext context = reopened->MakeSceneContext();
		CHECK(context.AssetManager.lock() == reopened->GetAssetManager());
		CHECK(context.Renderer2D.SortingLayers.GetLayerOrder("Particles") == 150);
	}

	TEST_CASE("EditorProjectSession Open returns nullptr for invalid projects")
	{
		Test::ScopedTempDirectory tempDirectory;

		SUBCASE("missing project file")
		{
			CHECK_FALSE(EditorProjectSession::Open(tempDirectory.GetPath() / "Missing.yproj"));
		}

		SUBCASE("malformed project file")
		{
			const auto projectFile = tempDirectory.WriteFile("Broken.yproj", "Project: [unclosed\n");
			CHECK_FALSE(EditorProjectSession::Open(projectFile));
		}

		SUBCASE("missing asset directory")
		{
			const auto projectFile = tempDirectory.WriteFile("Game.yproj", "Project:\n  Name: Game\n  AssetDirectory: Missing\n");
			CHECK_FALSE(EditorProjectSession::Open(projectFile));
			CHECK_FALSE(std::filesystem::exists(tempDirectory.GetPath() / "Missing"));
		}
	}

	TEST_CASE("EditorProjectSession Create returns nullptr when the asset directory cannot be created")
	{
		Test::ScopedTempDirectory tempDirectory;
		const auto blocker = tempDirectory.WriteFile("Blocker", "a file where the project directory should be");
		CHECK_FALSE(EditorProjectSession::Create(blocker / "Game.yproj"));
	}

	// 编辑器切换项目时新旧两个会话会同时存在
	TEST_CASE("EditorProjectSession writes its asset registry into its own project when destroyed")
	{
		Test::ScopedTempDirectory firstDirectory;
		Test::ScopedTempDirectory secondDirectory;
		Scope<EditorProjectSession> first = EditorProjectSession::Create(firstDirectory.GetPath() / "First.yproj");
		Scope<EditorProjectSession> second = EditorProjectSession::Create(secondDirectory.GetPath() / "Second.yproj");
		REQUIRE(first);
		REQUIRE(second);

		const std::filesystem::path firstRegistry = first->GetProject().GetAssetRegistryPath();
		const std::filesystem::path secondRegistry = second->GetProject().GetAssetRegistryPath();

		// 会话打开后才导入的资源只在内存注册表中，析构时才写回
		const std::filesystem::path scenePath = firstDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		REQUIRE((uint64_t)first->GetAssetManager()->ImportAsset(scenePath) != 0);
		CHECK(ReadTextFile(firstRegistry).find("Scenes/Level.yui") == std::string::npos);

		first.reset();

		CHECK(ReadTextFile(firstRegistry).find("Scenes/Level.yui") != std::string::npos);
		CHECK(ReadTextFile(secondRegistry).find("Level.yui") == std::string::npos);
	}
}
