#include "pch.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/RuntimeAssetManager.h"

#include <fstream>
#include <sstream>

using namespace Yuicy;
using Yuicy::Test::MakeAssetMetadata;

namespace {

	std::string ReadTextFile(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream content;
		content << stream.rdbuf();
		return content.str();
	}

}

TEST_SUITE("Asset")
{
	TEST_CASE("RuntimeAssetManager loads registered assets through its loaders")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / "Assets";
		const std::filesystem::path registryPath = assetDirectory / "AssetRegistry.yregistry";
		tempDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		tempDirectory.WriteFile("Assets/Scripts/Player.lua", "-- test\n");
		const AssetHandle sceneHandle = 100;
		const AssetHandle scriptHandle = 200;
		const AssetMetadata scene = MakeAssetMetadata(sceneHandle, AssetType::Scene, "Scenes/Level.yui");
		const AssetMetadata script = MakeAssetMetadata(scriptHandle, AssetType::LuaScript, "Scripts/Player.lua");
		Test::WriteAssetRegistry(registryPath, { scene, script });

		std::vector<std::filesystem::path> loadedPaths;
		Ref<AssetLoaderRegistry> loaders = CreateRef<AssetLoaderRegistry>();
		loaders->Register(AssetType::Scene, CreateScope<Test::RecordingAssetLoader>(loadedPaths));
		RuntimeAssetManager assetManager({ assetDirectory, registryPath, loaders });

		CHECK(assetManager.IsAssetHandleValid(sceneHandle));
		CHECK(assetManager.GetAssetType(sceneHandle) == AssetType::Scene);
		CHECK(assetManager.GetAssetType(scriptHandle) == AssetType::LuaScript);
		CHECK_FALSE(assetManager.IsAssetLoaded(sceneHandle));

		SUBCASE("loader receives the absolute path and the result is cached")
		{
			const Ref<Asset> asset = assetManager.GetAsset(sceneHandle);
			REQUIRE(asset);
			CHECK((uint64_t)asset->handle == (uint64_t)sceneHandle);
			REQUIRE(loadedPaths.size() == 1);
			CHECK(loadedPaths[0] == assetDirectory / "Scenes/Level.yui");
			CHECK(assetManager.IsAssetLoaded(sceneHandle));

			CHECK(assetManager.GetAsset(sceneHandle) == asset);
			CHECK(loadedPaths.size() == 1);
		}

		SUBCASE("asset types without a loader fail to load")
		{
			CHECK(assetManager.GetAsset(scriptHandle) == nullptr);
			CHECK_FALSE(assetManager.IsAssetLoaded(scriptHandle));
		}

		SUBCASE("unregistered handles")
		{
			const AssetHandle unknown = 300;
			CHECK_FALSE(assetManager.IsAssetHandleValid(unknown));
			CHECK(assetManager.GetAssetType(unknown) == AssetType::None);
			CHECK(assetManager.GetAsset(unknown) == nullptr);
			CHECK(loadedPaths.empty());
		}
	}

	TEST_CASE("RuntimeAssetManager leaves the asset directory and registry untouched")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / "Assets";
		const std::filesystem::path registryPath = assetDirectory / "AssetRegistry.yregistry";
		tempDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		tempDirectory.WriteFile("Assets/Scenes/Unregistered.yui", "Scene: Unregistered\nEntities: []\n");
		Test::WriteAssetRegistry(registryPath, { MakeAssetMetadata(100, AssetType::Scene, "Scenes/Level.yui") });
		const std::string registryBefore = ReadTextFile(registryPath);

		{
			RuntimeAssetManager assetManager({ assetDirectory, registryPath, CreateRef<AssetLoaderRegistry>() });
			CHECK(assetManager.IsAssetHandleValid(100));
			assetManager.AddMemoryOnlyAsset(CreateRef<Asset>());
		}

		CHECK(ReadTextFile(registryPath) == registryBefore);
	}

	TEST_CASE("RuntimeAssetManager keeps registered handles whose files are missing")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / "Assets";
		const std::filesystem::path registryPath = tempDirectory.GetPath() / "AssetRegistry.yregistry";
		Test::WriteAssetRegistry(registryPath, { MakeAssetMetadata(100, AssetType::Scene, "Scenes/Deleted.yui") });

		RuntimeAssetManager assetManager({ assetDirectory, registryPath, CreateRef<AssetLoaderRegistry>() });

		CHECK(assetManager.IsAssetHandleValid(100));
		CHECK(assetManager.GetAssetType(100) == AssetType::Scene);
	}

	TEST_CASE("RuntimeAssetManager without a registry serves memory-only assets")
	{
		RuntimeAssetManager assetManager({ {}, {}, CreateRef<AssetLoaderRegistry>() });

		const Ref<Asset> asset = CreateRef<Asset>();
		const AssetHandle handle = assetManager.AddMemoryOnlyAsset(asset);

		CHECK((uint64_t)handle != 0);
		CHECK((uint64_t)asset->handle == (uint64_t)handle);
		CHECK(assetManager.IsAssetHandleValid(handle));
		CHECK(assetManager.GetAsset(handle) == asset);
		CHECK(assetManager.GetAssetAs<Asset>(handle) == asset);
		CHECK_FALSE(assetManager.IsAssetLoaded(handle));
	}

	TEST_CASE("RuntimeAssetManager starts empty when the registry cannot be read")
	{
		Test::ScopedTempDirectory tempDirectory;

		SUBCASE("missing file")
		{
			RuntimeAssetManager assetManager({ tempDirectory.GetPath(), tempDirectory.GetPath() / "Missing.yregistry", CreateRef<AssetLoaderRegistry>() });
			CHECK_FALSE(assetManager.IsAssetHandleValid(100));
		}

		SUBCASE("malformed file")
		{
			const auto registryPath = tempDirectory.WriteFile("AssetRegistry.yregistry", "Assets: [unclosed\n");
			RuntimeAssetManager assetManager({ tempDirectory.GetPath(), registryPath, CreateRef<AssetLoaderRegistry>() });
			CHECK_FALSE(assetManager.IsAssetHandleValid(100));
		}
	}
}
