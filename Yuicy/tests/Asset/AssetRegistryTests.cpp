#include "pch.h"

#include "Yuicy/Asset/AssetRegistry.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"

using namespace Yuicy;

namespace {

	AssetMetadata MakeMetadata(AssetHandle handle, AssetType type, const std::filesystem::path& filePath)
	{
		AssetMetadata metadata;
		metadata.handle = handle;
		metadata.type = type;
		metadata.filePath = filePath;
		return metadata;
	}

}

TEST_SUITE("Asset")
{
	TEST_CASE("AssetRegistry set query and remove")
	{
		AssetRegistry registry;
		CHECK(registry.Count() == 0);

		registry.Set(100, MakeMetadata(100, AssetType::Scene, "Scenes/A.yui"));
		registry.Set(200, MakeMetadata(200, AssetType::Texture, "Textures/B.png"));

		CHECK(registry.Count() == 2);
		CHECK(registry.Contains(100));
		CHECK_FALSE(registry.Contains(300));
		CHECK(registry.Get(200).type == AssetType::Texture);
		CHECK(registry.Get(200).filePath == std::filesystem::path("Textures/B.png"));

		SUBCASE("Set overwrites existing entry")
		{
			registry.Set(100, MakeMetadata(100, AssetType::Scene, "Scenes/Renamed.yui"));
			CHECK(registry.Count() == 2);
			CHECK(registry.Get(100).filePath == std::filesystem::path("Scenes/Renamed.yui"));
		}

		SUBCASE("Remove returns erased count")
		{
			CHECK(registry.Remove(100) == 1);
			CHECK(registry.Remove(100) == 0);
			CHECK_FALSE(registry.Contains(100));
			CHECK(registry.Count() == 1);
		}

		SUBCASE("Clear")
		{
			registry.Clear();
			CHECK(registry.Count() == 0);
			CHECK(registry.begin() == registry.end());
		}
	}

	TEST_CASE("EditorAssetManager registry persists across project reopen")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / Test::ScopedActiveProject::kAssetDirectoryName;
		tempDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		tempDirectory.WriteFile("Assets/Scripts/Player.lua", "-- test\n");
		tempDirectory.WriteFile("Assets/readme.txt", "unknown extension");

		AssetHandle sceneHandle = 0;
		AssetHandle scriptHandle = 0;
		{
			Test::ScopedActiveProject project(tempDirectory.GetPath());
			auto assetManager = Project::GetEditorAssetManager();
			REQUIRE(assetManager);

			sceneHandle = assetManager->GetAssetHandleFromFilePath(assetDirectory / "Scenes/Level.yui");
			scriptHandle = assetManager->GetAssetHandleFromFilePath(assetDirectory / "Scripts/Player.lua");
			CHECK((uint64_t)sceneHandle != 0);
			CHECK((uint64_t)scriptHandle != 0);
			CHECK(assetManager->GetAssetType(sceneHandle) == AssetType::Scene);
			CHECK(assetManager->GetAssetType(scriptHandle) == AssetType::LuaScript);
			CHECK((uint64_t)assetManager->GetAssetHandleFromFilePath(assetDirectory / "readme.txt") == 0);
			CHECK(assetManager->GetAssetRegistry().Count() == 2);

			// 同一路径重复导入必须返回已有句柄，否则场景里保存的 AssetHandle 会失效
			CHECK((uint64_t)assetManager->ImportAsset(assetDirectory / "Scenes/Level.yui") == (uint64_t)sceneHandle);
			CHECK(assetManager->GetMetadata(sceneHandle).filePath == std::filesystem::path("Scenes/Level.yui"));
		}

		CHECK(std::filesystem::exists(assetDirectory / "AssetRegistry.yregistry"));

		Test::ScopedActiveProject reopened(tempDirectory.GetPath());
		auto assetManager = Project::GetEditorAssetManager();
		CHECK(assetManager->GetAssetRegistry().Count() == 2);
		CHECK((uint64_t)assetManager->GetAssetHandleFromFilePath(assetDirectory / "Scenes/Level.yui") == (uint64_t)sceneHandle);
		CHECK((uint64_t)assetManager->GetAssetHandleFromFilePath(assetDirectory / "Scripts/Player.lua") == (uint64_t)scriptHandle);
	}
}
