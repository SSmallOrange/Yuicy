#pragma once

#include "Yuicy/Asset/AssetImporter.h"
#include "Yuicy/Asset/AssetRegistry.h"

#include <filesystem>
#include <unordered_set>

namespace Yuicy {

	class EditorAssetManager
	{
	public:
		EditorAssetManager();
		~EditorAssetManager();

		// 资源查询
		AssetType GetAssetType(AssetHandle assetHandle);
		Ref<Asset> GetAsset(AssetHandle assetHandle);		// 懒加载

		template<typename T>
		Ref<T> GetAsset(AssetHandle assetHandle)
		{
			Ref<Asset> asset = GetAsset(assetHandle);
			return std::dynamic_pointer_cast<T>(asset);
		}

		bool IsAssetHandleValid(AssetHandle assetHandle);
		bool IsAssetLoaded(AssetHandle assetHandle);
		bool IsMemoryAsset(AssetHandle assetHandle);

		// 资源管理
		AssetHandle ImportAsset(const std::filesystem::path& filepath);  // 只导入、不Load
		bool ReloadData(AssetHandle assetHandle);
		void RemoveAsset(AssetHandle assetHandle);
		void AddMemoryOnlyAsset(Ref<Asset> asset);						 // 只存储句柄，不初始化资源

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
