#include "pch.h"

#include "Yuicy/Asset/AssetRegistrySerializer.h"

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
	TEST_CASE("AssetRegistrySerializer round-trip preserves every entry")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path registryPath = tempDirectory.GetPath() / "AssetRegistry.yregistry";

		AssetRegistry source;
		source.Set(300, MakeAssetMetadata(300, AssetType::LuaScript, "Scripts/Player.lua"));
		source.Set(100, MakeAssetMetadata(100, AssetType::Texture, std::filesystem::path(u8"Textures/角色 1.png")));
		source.Set(200, MakeAssetMetadata(200, AssetType::Scene, "Scenes/Level.yui"));
		REQUIRE(AssetRegistrySerializer::Serialize(source, registryPath));

		const std::optional<AssetRegistry> loaded = AssetRegistrySerializer::Deserialize(registryPath);
		REQUIRE(loaded);
		REQUIRE(loaded->Count() == source.Count());
		for (const auto& [handle, metadata] : source)
		{
			CAPTURE((uint64_t)handle);
			REQUIRE(loaded->Contains(handle));
			CHECK(loaded->Get(handle).type == metadata.type);
			CHECK(loaded->Get(handle).filePath == metadata.filePath);
			CHECK_FALSE(loaded->Get(handle).isDataLoaded);
		}

		const std::string text = ReadTextFile(registryPath);
		const size_t first = text.find("Handle: 100");
		const size_t second = text.find("Handle: 200");
		const size_t third = text.find("Handle: 300");
		REQUIRE(first != std::string::npos);
		CHECK(first < second);
		CHECK(second < third);
	}

	TEST_CASE("AssetRegistrySerializer round-trip of an empty registry")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path registryPath = tempDirectory.GetPath() / "AssetRegistry.yregistry";
		REQUIRE(AssetRegistrySerializer::Serialize(AssetRegistry(), registryPath));

		const std::optional<AssetRegistry> loaded = AssetRegistrySerializer::Deserialize(registryPath);
		REQUIRE(loaded);
		CHECK(loaded->Count() == 0);
	}

	TEST_CASE("AssetRegistrySerializer skips entries with handle 0 or an unknown type")
	{
		Test::ScopedTempDirectory tempDirectory;
		const auto path = tempDirectory.WriteFile("AssetRegistry.yregistry",
			"Assets:\n"
			"  - { Handle: 0, FilePath: Scenes/Zero.yui, Type: Scene }\n"
			"  - { Handle: 10, FilePath: Audio/Hit.wav, Type: AudioClip }\n"
			"  - { Handle: 20, FilePath: Scenes/Level.yui, Type: Scene }\n");

		const std::optional<AssetRegistry> loaded = AssetRegistrySerializer::Deserialize(path);
		REQUIRE(loaded);
		CHECK(loaded->Count() == 1);
		CHECK(loaded->Contains(20));
	}

	TEST_CASE("AssetRegistrySerializer rejects invalid registry files")
	{
		Test::ScopedTempDirectory tempDirectory;

		SUBCASE("missing file")
		{
			CHECK_FALSE(AssetRegistrySerializer::Deserialize(tempDirectory.GetPath() / "Missing.yregistry"));
		}

		SUBCASE("malformed YAML")
		{
			const auto path = tempDirectory.WriteFile("Broken.yregistry", "Assets: [unclosed\n");
			CHECK_FALSE(AssetRegistrySerializer::Deserialize(path));
		}

		SUBCASE("missing Assets")
		{
			const auto path = tempDirectory.WriteFile("NoAssets.yregistry", "Scene: Foo\n");
			CHECK_FALSE(AssetRegistrySerializer::Deserialize(path));
		}

		SUBCASE("Assets is not a sequence")
		{
			const auto path = tempDirectory.WriteFile("NotSequence.yregistry", "Assets: { Handle: 1 }\n");
			CHECK_FALSE(AssetRegistrySerializer::Deserialize(path));
		}

		SUBCASE("entry without FilePath")
		{
			const auto path = tempDirectory.WriteFile("NoPath.yregistry", "Assets:\n  - { Handle: 1, Type: Scene }\n");
			CHECK_FALSE(AssetRegistrySerializer::Deserialize(path));
		}
	}
}
