#include "pch.h"

#include "Editor/EditorProjectSession.h"
#include "Editor/Asset/EditorAssetManager.h"

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
	TEST_CASE("EditorProjectSession keeps the settings after Create then Save then Open")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectFile = tempDirectory.GetPath() / "Game.yproj";

		{
			Scope<EditorProjectSession> session = EditorProjectSession::Create(projectFile);
			REQUIRE(session);
			CHECK(std::filesystem::is_regular_file(projectFile));
			CHECK(std::filesystem::is_directory(session->GetProject().GetAssetDirectory()));
			CHECK(std::filesystem::is_directory(session->GetProject().GetAssetDirectory() / "Scripts"));
			CHECK(session->GetProject().GetSettings().Name == "Game");

			ProjectSettings& settings = session->GetProject().GetSettings();
			settings.StartScene = "Scenes/Main.yui";
			settings.Renderer2D.SortingLayers.AddLayer("Particles", 150);
			settings.Physics2D.CollisionLayers.LayerNames[3] = "Enemy";
			REQUIRE(session->Save());
		}

		Scope<EditorProjectSession> reopened = EditorProjectSession::Open(projectFile);
		REQUIRE(reopened);
		CHECK(reopened->GetProject().GetProjectFile() == projectFile.lexically_normal());

		const ProjectSettings& settings = reopened->GetProject().GetSettings();
		CHECK(settings.Name == "Game");
		CHECK(settings.StartScene == "Scenes/Main.yui");
		CHECK(settings.Renderer2D.SortingLayers.GetLayerOrder("Particles") == 150);
		CHECK(settings.Physics2D.CollisionLayers.LayerNames[3] == "Enemy");

		REQUIRE(reopened->GetAssetManager());
		CHECK(reopened->GetAssetManager()->GetAssetDirectory() == reopened->GetProject().GetAssetDirectory());

		const SceneContext context = reopened->MakeSceneContext();
		CHECK(context.AssetManager.lock() == reopened->GetAssetManager());
		CHECK(context.Renderer2D.SortingLayers.GetLayerOrder("Particles") == 150);
	}

	TEST_CASE("EditorProjectSession keeps editor user settings out of the project file")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectFile = tempDirectory.GetPath() / "Game.yproj";
		const std::filesystem::path userSettingsFile = EditorProjectUserSettingsSerializer::GetFilePath(tempDirectory.GetPath());

		{
			Scope<EditorProjectSession> session = EditorProjectSession::Create(projectFile);
			REQUIRE(session);
			CHECK(std::filesystem::is_regular_file(userSettingsFile));
			CHECK(std::filesystem::is_regular_file(userSettingsFile.parent_path() / ".gitignore"));
			CHECK(ReadTextFile(projectFile).find("AutoSave") == std::string::npos);
		}

		EditorProjectUserSettings userSettings;
		userSettings.autoSave.enabled = true;
		userSettings.autoSave.intervalSeconds = 42;
		REQUIRE(EditorProjectUserSettingsSerializer::Serialize(userSettings, tempDirectory.GetPath()));

		Scope<EditorProjectSession> reopened = EditorProjectSession::Open(projectFile);
		REQUIRE(reopened);
		CHECK(reopened->GetUserSettings().autoSave.enabled);
		CHECK(reopened->GetUserSettings().autoSave.intervalSeconds == 42);
	}

	TEST_CASE("EditorProjectSession Open uses default user settings when the file is missing or malformed")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path projectFile = tempDirectory.GetPath() / "Game.yproj";
		const std::filesystem::path userSettingsFile = EditorProjectUserSettingsSerializer::GetFilePath(tempDirectory.GetPath());
		REQUIRE(EditorProjectSession::Create(projectFile));

		const EditorAutoSaveSettings defaults;

		SUBCASE("missing file is generated")
		{
			std::filesystem::remove_all(userSettingsFile.parent_path());

			Scope<EditorProjectSession> session = EditorProjectSession::Open(projectFile);
			REQUIRE(session);
			CHECK(session->GetUserSettings().autoSave.enabled == defaults.enabled);
			CHECK(session->GetUserSettings().autoSave.intervalSeconds == defaults.intervalSeconds);
			CHECK(std::filesystem::is_regular_file(userSettingsFile));
		}

		SUBCASE("malformed file does not block opening the project")
		{
			tempDirectory.WriteFile(".yuistudio/EditorUserSettings.yaml", "AutoSave: [unclosed\n");

			Scope<EditorProjectSession> session = EditorProjectSession::Open(projectFile);
			REQUIRE(session);
			CHECK(session->GetUserSettings().autoSave.enabled == defaults.enabled);
			CHECK(session->GetUserSettings().autoSave.intervalSeconds == defaults.intervalSeconds);
		}
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
			CHECK_FALSE(std::filesystem::exists(EditorProjectUserSettingsSerializer::GetFilePath(tempDirectory.GetPath())));
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
