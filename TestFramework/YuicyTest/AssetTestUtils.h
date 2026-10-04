#pragma once

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/AssetRegistrySerializer.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <initializer_list>
#include <vector>

namespace Yuicy::Test {

	inline AssetMetadata MakeAssetMetadata(AssetHandle handle, AssetType type, const std::filesystem::path& filePath)
	{
		AssetMetadata metadata;
		metadata.handle = handle;
		metadata.type = type;
		metadata.filePath = filePath;
		return metadata;
	}

	// 写出一份只含 entries 的注册表文件；写入失败时终止当前用例
	inline void WriteAssetRegistry(const std::filesystem::path& registryPath, std::initializer_list<AssetMetadata> entries)
	{
		AssetRegistry registry;
		for (const AssetMetadata& metadata : entries)
			registry.Set(metadata.handle, metadata);

		REQUIRE(AssetRegistrySerializer::Serialize(registry, registryPath));
	}

	// 记录每次 Load 收到的绝对路径，返回空的 Asset；没有 GL 上下文与 Lua 状态时也能使用
	class RecordingAssetLoader : public AssetLoader
	{
	public:
		explicit RecordingAssetLoader(std::vector<std::filesystem::path>& loadedPaths)
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
