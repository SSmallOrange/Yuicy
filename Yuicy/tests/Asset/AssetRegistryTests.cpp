#include "pch.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/AssetRegistry.h"
#include "Yuicy/Asset/EditorAssetManager.h"

#include <fstream>
#include <sstream>

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

	// loaders 默认为空注册表，不会创建 GPU 或 Lua 资源
	EditorAssetManagerSpecification MakeSpecification(const std::filesystem::path& assetDirectory,
		Ref<const AssetLoaderRegistry> loaders = CreateRef<AssetLoaderRegistry>())
	{
		std::filesystem::create_directories(assetDirectory);
		return { assetDirectory, assetDirectory / "AssetRegistry.yregistry", std::move(loaders) };
	}

	std::string ReadTextFile(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		std::stringstream content;
		content << stream.rdbuf();
		return content.str();
	}

	class RecordingLoader : public AssetLoader
	{
	public:
		explicit RecordingLoader(std::vector<std::filesystem::path>& loadedPaths)
			: m_LoadedPaths(loadedPaths)
		{
		}

		Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const override
		{
			m_LoadedPaths.push_back(absolutePath);
			return CreateRef<Asset>();
		}

	private:
		std::vector<std::filesystem::path>& m_LoadedPaths;
	};
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

	TEST_CASE("AssetRegistry asserts on mismatched or zero handles")
	{
		AssetRegistry registry;
		Test::ScopedAssertCapture asserts;

		registry.Set(0, MakeMetadata(0, AssetType::Scene, "Scenes/A.yui"));
		CHECK(asserts.GetCount() == 1);
		CHECK(asserts.GetLastExpression() == "handle != 0");

		registry.Set(100, MakeMetadata(200, AssetType::Scene, "Scenes/B.yui"));
		CHECK(asserts.GetCount() == 2);
		CHECK(asserts.GetLastExpression() == "metadata.handle == handle");
	}

	TEST_CASE("EditorAssetManager registry persists across reopen")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / "Assets";
		tempDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		tempDirectory.WriteFile("Assets/Scripts/Player.lua", "-- test\n");
		tempDirectory.WriteFile("Assets/readme.txt", "unknown extension");

		AssetHandle sceneHandle = 0;
		AssetHandle scriptHandle = 0;
		{
			EditorAssetManager assetManager(MakeSpecification(assetDirectory));

			sceneHandle = assetManager.GetAssetHandleFromFilePath(assetDirectory / "Scenes/Level.yui");
			scriptHandle = assetManager.GetAssetHandleFromFilePath(assetDirectory / "Scripts/Player.lua");
			CHECK((uint64_t)sceneHandle != 0);
			CHECK((uint64_t)scriptHandle != 0);
			CHECK(assetManager.GetAssetType(sceneHandle) == AssetType::Scene);
			CHECK(assetManager.GetAssetType(scriptHandle) == AssetType::LuaScript);
			CHECK((uint64_t)assetManager.GetAssetHandleFromFilePath(assetDirectory / "readme.txt") == 0);
			CHECK(assetManager.GetAssetRegistry().Count() == 2);

			// 同一路径重复导入必须返回已有句柄，否则场景里保存的 AssetHandle 会失效
			CHECK((uint64_t)assetManager.ImportAsset(assetDirectory / "Scenes/Level.yui") == (uint64_t)sceneHandle);
			CHECK(assetManager.GetMetadata(sceneHandle).filePath == std::filesystem::path("Scenes/Level.yui"));
		}

		CHECK(std::filesystem::exists(assetDirectory / "AssetRegistry.yregistry"));

		EditorAssetManager reopened(MakeSpecification(assetDirectory));
		CHECK(reopened.GetAssetRegistry().Count() == 2);
		CHECK((uint64_t)reopened.GetAssetHandleFromFilePath(assetDirectory / "Scenes/Level.yui") == (uint64_t)sceneHandle);
		CHECK((uint64_t)reopened.GetAssetHandleFromFilePath(assetDirectory / "Scripts/Player.lua") == (uint64_t)scriptHandle);
	}

	TEST_CASE("Two EditorAssetManagers in different directories do not interfere")
	{
		Test::ScopedTempDirectory firstDirectory;
		Test::ScopedTempDirectory secondDirectory;
		const std::filesystem::path firstAssets = firstDirectory.GetPath() / "Assets";
		const std::filesystem::path secondAssets = secondDirectory.GetPath() / "Assets";
		firstDirectory.WriteFile("Assets/Scenes/First.yui", "Scene: First\nEntities: []\n");
		secondDirectory.WriteFile("Assets/Scripts/Second.lua", "-- test\n");

		{
			EditorAssetManager first(MakeSpecification(firstAssets));
			EditorAssetManager second(MakeSpecification(secondAssets));

			const AssetHandle firstHandle = first.GetAssetHandleFromFilePath(firstAssets / "Scenes/First.yui");
			const AssetHandle secondHandle = second.GetAssetHandleFromFilePath(secondAssets / "Scripts/Second.lua");
			REQUIRE((uint64_t)firstHandle != 0);
			REQUIRE((uint64_t)secondHandle != 0);

			CHECK(first.GetAssetRegistry().Count() == 1);
			CHECK(second.GetAssetRegistry().Count() == 1);
			CHECK_FALSE(first.IsAssetHandleValid(secondHandle));
			CHECK_FALSE(second.IsAssetHandleValid(firstHandle));
			CHECK(first.GetFileSystemPath(firstHandle) == firstAssets / "Scenes/First.yui");
			CHECK(second.GetFileSystemPath(secondHandle) == secondAssets / "Scripts/Second.lua");

			const AssetHandle memoryHandle = first.AddMemoryOnlyAsset(CreateRef<Asset>());
			CHECK(first.IsAssetHandleValid(memoryHandle));
			CHECK_FALSE(second.IsAssetHandleValid(memoryHandle));
		}

		// 注册表在析构时写回
		const std::string firstRegistry = ReadTextFile(firstAssets / "AssetRegistry.yregistry");
		const std::string secondRegistry = ReadTextFile(secondAssets / "AssetRegistry.yregistry");
		CHECK(firstRegistry.find("Scenes/First.yui") != std::string::npos);
		CHECK(firstRegistry.find("Second.lua") == std::string::npos);
		CHECK(secondRegistry.find("Scripts/Second.lua") != std::string::npos);
		CHECK(secondRegistry.find("First.yui") == std::string::npos);
	}

	TEST_CASE("EditorAssetManager GetRelativePath strips only its own asset directory")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path root = tempDirectory.GetPath();
		const std::filesystem::path assetDirectory = root / "Assets";
		EditorAssetManager assetManager(MakeSpecification(assetDirectory));

		CHECK(assetManager.GetRelativePath(assetDirectory / "Textures/Hero.png") == std::filesystem::path("Textures/Hero.png"));
		CHECK(assetManager.GetRelativePath(assetDirectory / "Textures/../Scenes/Level.yui") == std::filesystem::path("Scenes/Level.yui"));

		// 资产目录外的路径返回规范化后的原路径
		CHECK(assetManager.GetRelativePath(root / "Other/Hero.png") == (root / "Other/Hero.png").lexically_normal());
		CHECK(assetManager.GetRelativePath(assetDirectory / "../Outside.png") == (root / "Outside.png").lexically_normal());
		// 按路径分量比较，而不是按字符串前缀
		CHECK(assetManager.GetRelativePath(root / "AssetsBackup/Hero.png") == (root / "AssetsBackup/Hero.png").lexically_normal());

		// 注册表中保存的是相对路径
		CHECK(assetManager.GetRelativePath("Textures/Hero.png") == std::filesystem::path("Textures/Hero.png"));
	}

	TEST_CASE("EditorAssetManager loads assets through its injected loaders")
	{
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path assetDirectory = tempDirectory.GetPath() / "Assets";
		tempDirectory.WriteFile("Assets/Scenes/Level.yui", "Scene: Level\nEntities: []\n");
		tempDirectory.WriteFile("Assets/Scripts/Player.lua", "-- test\n");

		std::vector<std::filesystem::path> loadedPaths;
		Ref<AssetLoaderRegistry> loaders = CreateRef<AssetLoaderRegistry>();
		loaders->Register(AssetType::Scene, CreateScope<RecordingLoader>(loadedPaths));

		EditorAssetManager assetManager(MakeSpecification(assetDirectory, loaders));
		const AssetHandle sceneHandle = assetManager.GetAssetHandleFromFilePath(assetDirectory / "Scenes/Level.yui");
		const AssetHandle scriptHandle = assetManager.GetAssetHandleFromFilePath(assetDirectory / "Scripts/Player.lua");

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

		SUBCASE("ReloadData calls the loader again and replaces the cached asset")
		{
			const Ref<Asset> original = assetManager.GetAsset(sceneHandle);
			REQUIRE(original);
			CHECK(assetManager.ReloadData(sceneHandle));
			CHECK(loadedPaths.size() == 2);
			CHECK(assetManager.GetAsset(sceneHandle) != original);
		}

		SUBCASE("asset types without a loader fail to load")
		{
			CHECK(assetManager.GetAsset(scriptHandle) == nullptr);
			CHECK_FALSE(assetManager.IsAssetLoaded(scriptHandle));
			CHECK(loadedPaths.empty());
		}
	}

	TEST_CASE("AssetLoaderRegistry asserts on a second loader for the same type")
	{
		std::vector<std::filesystem::path> loadedPaths;
		AssetLoaderRegistry registry;
		registry.Register(AssetType::Scene, CreateScope<RecordingLoader>(loadedPaths));

		Test::ScopedAssertCapture asserts;
		registry.Register(AssetType::Scene, CreateScope<RecordingLoader>(loadedPaths));
		CHECK(asserts.GetCount() == 1);
	}
}
