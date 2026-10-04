#pragma once

#include "Yuicy/Asset/AssetManagerBase.h"
#include "Yuicy/Asset/AssetRegistry.h"

#include <filesystem>
#include <unordered_map>

namespace Yuicy {

	class AssetLoaderRegistry;

	struct RuntimeAssetManagerSpecification
	{
		std::filesystem::path AssetDirectory;
		std::filesystem::path RegistryPath;  // 空表示没有注册表，只有 AddMemoryOnlyAsset 登记的资源
		Ref<const AssetLoaderRegistry> Loaders;
	};

	// 登记信息来自构造时读取的注册表与 AddMemoryOnlyAsset；资源在首次 GetAsset 时加载
	// IsAssetHandleValid 只反映登记情况，文件缺失要到 GetAsset 加载时才会发现
	class RuntimeAssetManager : public AssetManagerBase
	{
	public:
		// 注册表文件无法读取时记录错误，按空注册表继续
		explicit RuntimeAssetManager(RuntimeAssetManagerSpecification specification);

		Ref<Asset> GetAsset(AssetHandle assetHandle) override;
		AssetType GetAssetType(AssetHandle assetHandle) const override;
		bool IsAssetHandleValid(AssetHandle assetHandle) const override;
		// 只统计从文件加载的资源，内存资源返回 false
		bool IsAssetLoaded(AssetHandle assetHandle) const override;
		AssetHandle AddMemoryOnlyAsset(const Ref<Asset>& asset) override;

	private:
		RuntimeAssetManagerSpecification m_specification;
		AssetRegistry m_assetRegistry;
		std::unordered_map<AssetHandle, Ref<Asset>> m_loadedAssets;
		std::unordered_map<AssetHandle, Ref<Asset>> m_memoryAssets;
	};

}
