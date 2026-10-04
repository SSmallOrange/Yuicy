#include "pch.h"
#include "EditorAssetManager.h"

#include "Yuicy/Asset/AssetExtensions.h"
#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/AssetRegistrySerializer.h"
#include "Yuicy/Core/Log.h"

namespace Yuicy {

	static AssetMetadata s_nullMetadata;

	EditorAssetManager::EditorAssetManager(EditorAssetManagerSpecification specification)
		: m_specification(std::move(specification))
	{
		YUICY_ASSERT(m_specification.Loaders, "EditorAssetManager requires an AssetLoaderRegistry");
		LoadAssetRegistry();
		ReloadAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle) const
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		if (auto it = m_memoryAssets.find(assetHandle); it != m_memoryAssets.end())
			return it->second->GetAssetType();

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		// 先检查 memory-only 资源
		if (auto it = m_memoryAssets.find(assetHandle); it != m_memoryAssets.end())
			return it->second;

		// 检查元数据是否有效
		const auto& metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return nullptr;

		// 直接返回已加载资源
		if (metadata.isDataLoaded)
		{
			if (auto it = m_loadedAssets.find(assetHandle); it != m_loadedAssets.end())
				return it->second;
		}

		// 未加载
		Ref<Asset> asset = LoadAssetData(metadata);
		if (asset)
		{
			AssetMetadata updatedMetadata = metadata;
			updatedMetadata.isDataLoaded = true;
			m_assetRegistry.Set(assetHandle, updatedMetadata);

			m_loadedAssets[assetHandle] = asset;

			YUICY_INFO("[AssetManager] Loaded asset: {0}", metadata.filePath.string());
		}
		else
		{
			YUICY_ERROR("[AssetManager] Failed to load asset: {0}", metadata.filePath.string());
		}

		return asset;
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle assetHandle) const
	{
		return (m_memoryAssets.find(assetHandle) != m_memoryAssets.end())
			|| GetMetadata(assetHandle).IsValid();
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle assetHandle) const
	{
		return m_loadedAssets.find(assetHandle) != m_loadedAssets.end();
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle assetHandle) const
	{
		return m_memoryAssets.find(assetHandle) != m_memoryAssets.end();
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		auto relativePath = GetRelativePath(filepath);

		// 已存在该路径的资源 → 直接返回
		const auto& existingMetadata = GetMetadata(relativePath);
		if (existingMetadata.IsValid())
			return existingMetadata.handle;

		// 识别资源类型
		AssetType type = GetAssetTypeFromPath(relativePath);
		if (type == AssetType::None)
			return 0;

		// 创建新元数据
		AssetMetadata metadata;
		metadata.handle = AssetHandle(); // 生成新的 UUID
		metadata.filePath = relativePath;
		metadata.type = type;

		m_assetRegistry.Set(metadata.handle, metadata);

		return metadata.handle;
	}

	bool EditorAssetManager::ReloadData(AssetHandle assetHandle)
	{
		const auto& metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			YUICY_ERROR("[AssetManager] Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = LoadAssetData(metadata);
		AssetMetadata updatedMetadata = metadata;
		updatedMetadata.isDataLoaded = asset != nullptr;

		if (updatedMetadata.isDataLoaded)
		{
			m_loadedAssets[assetHandle] = asset;
			m_assetRegistry.Set(assetHandle, updatedMetadata);
			YUICY_INFO("[AssetManager] Reloaded asset: {0}", updatedMetadata.filePath.string());
		}
		else
		{
			YUICY_ERROR("[AssetManager] Failed to reload asset: {0}", updatedMetadata.filePath.string());
		}

		return updatedMetadata.isDataLoaded;
	}

	void EditorAssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		m_memoryAssets.erase(assetHandle);
		m_loadedAssets.erase(assetHandle);

		if (m_assetRegistry.Contains(assetHandle))
			m_assetRegistry.Remove(assetHandle);
	}

	AssetHandle EditorAssetManager::AddMemoryOnlyAsset(const Ref<Asset>& asset)
	{
		asset->handle = AssetHandle();
		m_memoryAssets[asset->handle] = asset;
		return asset->handle;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		return GetMetadata(filepath).handle;
	}

	AssetType EditorAssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (const auto it = s_assetExtensionMap.find(ext); it != s_assetExtensionMap.end())
			return it->second;

		return AssetType::None;
	}

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(AssetHandle assetHandle) const
	{
		return GetFileSystemPath(GetMetadata(assetHandle));
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata) const
	{
		return m_specification.AssetDirectory / metadata.filePath;
	}

	std::filesystem::path EditorAssetManager::GetRelativePath(const std::filesystem::path& filepath) const
	{
		const auto normalizedFile = filepath.lexically_normal();
		const auto normalizedAssetDir = m_specification.AssetDirectory.lexically_normal();

		// 判断：filepath 是否位于 assetDir 下
		auto fileIt = normalizedFile.begin();
		auto dirIt = normalizedAssetDir.begin();

		for (; dirIt != normalizedAssetDir.end() && fileIt != normalizedFile.end(); ++dirIt, ++fileIt)
		{
			if (*dirIt != *fileIt)
				return normalizedFile; // 不是子路径，直接返回规范化后的原路径
		}

		// assetDir 还没比较完，说明 filepath 比 assetDir 还短，不可能是其子路径
		if (dirIt != normalizedAssetDir.end())
			return normalizedFile;

		// 走到这里，说明 normalizedFile == normalizedAssetDir
		// 或 normalizedFile 位于 normalizedAssetDir 下面
		auto relativePath = normalizedFile.lexically_relative(normalizedAssetDir);

		// 两者相同，会得到 "."
		// 这里一般不算空；但保留兜底更稳
		return relativePath.empty() ? normalizedFile : relativePath;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle assetHandle) const
	{
		if (m_assetRegistry.Contains(assetHandle))
			return m_assetRegistry.Get(assetHandle);

		return s_nullMetadata;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(const std::filesystem::path& filepath) const
	{
		const auto relativePath = GetRelativePath(filepath);

		for (const auto& [handle, metadata] : m_assetRegistry)
		{
			if (metadata.filePath == relativePath)
				return metadata;
		}

		return s_nullMetadata;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		// memory-only 资源
		for (const auto& [handle, asset] : m_memoryAssets)
		{
			if (asset->GetAssetType() == type)
				result.insert(handle);
		}

		// 注册表资源
		for (const auto& [handle, metadata] : m_assetRegistry)
		{
			if (metadata.type == type)
				result.insert(handle);
		}

		return result;
	}

	Ref<Asset> EditorAssetManager::LoadAssetData(const AssetMetadata& metadata) const
	{
		if (!m_specification.Loaders)
			return nullptr;

		return m_specification.Loaders->Load(metadata, GetFileSystemPath(metadata));
	}

	// 注册表持久化
	void EditorAssetManager::LoadAssetRegistry()
	{
		YUICY_INFO("[AssetManager] Loading Asset Registry");

		if (!std::filesystem::exists(m_specification.RegistryPath))
			return;

		std::optional<AssetRegistry> registry = AssetRegistrySerializer::Deserialize(m_specification.RegistryPath);
		if (!registry)
			return;

		for (const auto& [handle, storedMetadata] : *registry)
		{
			AssetMetadata metadata = storedMetadata;

			// 验证扩展名和记录的类型是否匹配
			const AssetType typeFromPath = GetAssetTypeFromPath(metadata.filePath);
			if (metadata.type != typeFromPath)
			{
				YUICY_WARN("[AssetManager] Mismatch between stored AssetType and extension type!");
				metadata.type = typeFromPath;
			}

			// 检查文件是否依然存在
			if (!std::filesystem::exists(GetFileSystemPath(metadata)))
			{
				YUICY_WARN("[AssetManager] Missing asset '{0}' detected in registry", metadata.filePath.string());
				continue;
			}

			m_assetRegistry.Set(handle, metadata);
		}

		YUICY_INFO("[AssetManager] Loaded {0} asset entries", m_assetRegistry.Count());
	}

	void EditorAssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void EditorAssetManager::ReloadAssets()
	{
		ProcessDirectory(m_specification.AssetDirectory);
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		AssetRegistry existingAssets;
		for (const auto& [handle, metadata] : m_assetRegistry)
		{
			if (std::filesystem::exists(GetFileSystemPath(metadata)))
				existingAssets.Set(handle, metadata);
		}

		YUICY_INFO("[AssetManager] Serializing asset registry with {0} entries", existingAssets.Count());
		AssetRegistrySerializer::Serialize(existingAssets, m_specification.RegistryPath);
	}

}
