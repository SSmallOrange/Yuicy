#include "pch.h"
#include "Yuicy/Asset/RuntimeAssetManager.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/AssetRegistrySerializer.h"

namespace Yuicy {

	RuntimeAssetManager::RuntimeAssetManager(RuntimeAssetManagerSpecification specification)
		: m_specification(std::move(specification))
	{
		YUICY_CORE_ASSERT(m_specification.Loaders, "RuntimeAssetManager requires an AssetLoaderRegistry");

		if (m_specification.RegistryPath.empty())
			return;

		if (std::optional<AssetRegistry> registry = AssetRegistrySerializer::Deserialize(m_specification.RegistryPath))
			m_assetRegistry = std::move(*registry);
	}

	Ref<Asset> RuntimeAssetManager::GetAsset(AssetHandle assetHandle)
	{
		if (auto it = m_memoryAssets.find(assetHandle); it != m_memoryAssets.end())
			return it->second;

		if (auto it = m_loadedAssets.find(assetHandle); it != m_loadedAssets.end())
			return it->second;

		if (!m_assetRegistry.Contains(assetHandle) || !m_specification.Loaders)
			return nullptr;

		const AssetMetadata& metadata = m_assetRegistry.Get(assetHandle);
		Ref<Asset> asset = m_specification.Loaders->Load(metadata, m_specification.AssetDirectory / metadata.filePath);
		if (!asset)
		{
			YUICY_CORE_ERROR("[RuntimeAssetManager] Failed to load asset: {}", metadata.filePath.string());
			return nullptr;
		}

		m_loadedAssets[assetHandle] = asset;
		return asset;
	}

	AssetType RuntimeAssetManager::GetAssetType(AssetHandle assetHandle) const
	{
		if (auto it = m_memoryAssets.find(assetHandle); it != m_memoryAssets.end())
			return it->second->GetAssetType();

		if (m_assetRegistry.Contains(assetHandle))
			return m_assetRegistry.Get(assetHandle).type;

		return AssetType::None;
	}

	bool RuntimeAssetManager::IsAssetHandleValid(AssetHandle assetHandle) const
	{
		return m_memoryAssets.contains(assetHandle) || m_assetRegistry.Contains(assetHandle);
	}

	bool RuntimeAssetManager::IsAssetLoaded(AssetHandle assetHandle) const
	{
		return m_loadedAssets.contains(assetHandle);
	}

	AssetHandle RuntimeAssetManager::AddMemoryOnlyAsset(const Ref<Asset>& asset)
	{
		asset->handle = AssetHandle();
		m_memoryAssets[asset->handle] = asset;
		return asset->handle;
	}

}
