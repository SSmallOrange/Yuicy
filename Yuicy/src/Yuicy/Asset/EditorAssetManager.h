#pragma once

#include "Yuicy/Asset/AssetImporter.h"
#include "Yuicy/Asset/AssetManagerBase.h"
#include "Yuicy/Asset/AssetRegistry.h"

#include <filesystem>
#include <unordered_set>

namespace Yuicy {

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		EditorAssetManager();
		~EditorAssetManager() override;

		// 资源查询
		Ref<Asset> GetAsset(AssetHandle assetHandle) override;

		AssetType GetAssetType(AssetHandle assetHandle) const override;
		bool IsAssetHandleValid(AssetHandle assetHandle) const override;
		bool IsAssetLoaded(AssetHandle assetHandle) const override;
		bool IsMemoryAsset(AssetHandle assetHandle) const;

		// 资源管理
		AssetHandle ImportAsset(const std::filesystem::path& filepath);  // 只导入、不Load
		bool ReloadData(AssetHandle assetHandle);
		void RemoveAsset(AssetHandle assetHandle);
		AssetHandle AddMemoryOnlyAsset(const Ref<Asset>& asset) override;

		// 路径工具
		AssetHandle GetAssetHandleFromFilePath(const std::filesystem::path& filepath);
		AssetType GetAssetTypeFromExtension(const std::string& extension);
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path);
		std::filesystem::path GetFileSystemPath(AssetHandle assetHandle);
		static std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata);
		static std::filesystem::path GetRelativePath(const std::filesystem::path& filepath);

		// 注册表
		const AssetRegistry& GetAssetRegistry() const { return m_assetRegistry; }
		const AssetMetadata& GetMetadata(AssetHandle assetHandle) const;
		const AssetMetadata& GetMetadata(const std::filesystem::path& filepath) const;
		std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		void WriteRegistryToFile();

	private:
		void LoadAssetRegistry();
		void ProcessDirectory(const std::filesystem::path& directoryPath);
		void ReloadAssets();

	private:
		std::unordered_map<AssetHandle, Ref<Asset>> m_loadedAssets;  // 有实体文件的资源
		std::unordered_map<AssetHandle, Ref<Asset>> m_memoryAssets;  // 无实体文件的资源
		AssetRegistry m_assetRegistry;
	};

}
