#pragma once

#include "Yuicy/Asset/AssetManagerBase.h"
#include "Yuicy/Asset/AssetMetadata.h"
#include "Yuicy/Asset/AssetRegistry.h"

#include <filesystem>
#include <unordered_set>

namespace Yuicy {

	class AssetLoaderRegistry;

	// 路径只做词法比较，不访问文件系统解析：调用方传入的文件路径要与 AssetDirectory 同为绝对路径或同为相对路径
	struct EditorAssetManagerSpecification
	{
		std::filesystem::path AssetDirectory;
		std::filesystem::path RegistryPath;
		Ref<const AssetLoaderRegistry> Loaders;
	};

	class EditorAssetManager : public AssetManagerBase
	{
	public:
		// 读取注册表并扫描 AssetDirectory，AssetDirectory 必须已存在；析构时把注册表写回 RegistryPath
		// 注册表无法解析时按空注册表处理，扫描到的文件都会分配新的 Handle
		explicit EditorAssetManager(EditorAssetManagerSpecification specification);
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
		const std::filesystem::path& GetAssetDirectory() const { return m_specification.AssetDirectory; }
		std::filesystem::path GetFileSystemPath(AssetHandle assetHandle) const;
		std::filesystem::path GetFileSystemPath(const AssetMetadata& metadata) const;
		// 位于资产目录内时返回相对资产目录的路径，否则返回词法规范化后的原路径
		std::filesystem::path GetRelativePath(const std::filesystem::path& filepath) const;

		// 注册表
		const AssetRegistry& GetAssetRegistry() const { return m_assetRegistry; }
		const AssetMetadata& GetMetadata(AssetHandle assetHandle) const;
		const AssetMetadata& GetMetadata(const std::filesystem::path& filepath) const;
		std::unordered_set<AssetHandle> GetAllAssetsWithType(AssetType type);

		void WriteRegistryToFile();

	private:
		Ref<Asset> LoadAssetData(const AssetMetadata& metadata) const;
		void LoadAssetRegistry();
		void ProcessDirectory(const std::filesystem::path& directoryPath);
		void ReloadAssets();

	private:
		EditorAssetManagerSpecification m_specification;
		std::unordered_map<AssetHandle, Ref<Asset>> m_loadedAssets;  // 有实体文件的资源
		std::unordered_map<AssetHandle, Ref<Asset>> m_memoryAssets;  // 无实体文件的资源
		AssetRegistry m_assetRegistry;
	};

}
