#include "pch.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/AssetRegistry.h"

using namespace Yuicy;
using Yuicy::Test::MakeAssetMetadata;

TEST_SUITE("Asset")
{
	TEST_CASE("AssetRegistry set query and remove")
	{
		AssetRegistry registry;
		CHECK(registry.Count() == 0);

		registry.Set(100, MakeAssetMetadata(100, AssetType::Scene, "Scenes/A.yui"));
		registry.Set(200, MakeAssetMetadata(200, AssetType::Texture, "Textures/B.png"));

		CHECK(registry.Count() == 2);
		CHECK(registry.Contains(100));
		CHECK_FALSE(registry.Contains(300));
		CHECK(registry.Get(200).type == AssetType::Texture);
		CHECK(registry.Get(200).filePath == std::filesystem::path("Textures/B.png"));

		SUBCASE("Set overwrites existing entry")
		{
			registry.Set(100, MakeAssetMetadata(100, AssetType::Scene, "Scenes/Renamed.yui"));
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

	TEST_CASE("AssetRegistry asserts on mismatched or zero handles")
	{
		AssetRegistry registry;
		Test::ScopedAssertCapture asserts;

		registry.Set(0, MakeAssetMetadata(0, AssetType::Scene, "Scenes/A.yui"));
		CHECK(asserts.GetCount() == 1);
		CHECK(asserts.GetLastExpression() == "handle != 0");

		registry.Set(100, MakeAssetMetadata(200, AssetType::Scene, "Scenes/B.yui"));
		CHECK(asserts.GetCount() == 2);
		CHECK(asserts.GetLastExpression() == "metadata.handle == handle");
	}

	TEST_CASE("AssetLoaderRegistry asserts on a second loader for the same type")
	{
		std::vector<std::filesystem::path> loadedPaths;
		AssetLoaderRegistry registry;
		registry.Register(AssetType::Scene, CreateScope<Test::RecordingAssetLoader>(loadedPaths));

		Test::ScopedAssertCapture asserts;
		registry.Register(AssetType::Scene, CreateScope<Test::RecordingAssetLoader>(loadedPaths));
		CHECK(asserts.GetCount() == 1);
	}
}
