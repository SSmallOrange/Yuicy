#include "pch.h"
#include "Yuicy/Asset/AssetLoader.h"

namespace Yuicy {

	void AssetLoaderRegistry::Register(AssetType type, Scope<AssetLoader> loader)
	{
		YUICY_CORE_ASSERT(type != AssetType::None, "Cannot register a loader for AssetType::None");
		YUICY_CORE_ASSERT(loader, "Loader must not be null");
		YUICY_CORE_ASSERT(!m_loaders.contains(type), "A loader is already registered for this asset type");

		m_loaders[type] = std::move(loader);
	}

	Ref<Asset> AssetLoaderRegistry::Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const
	{
		const auto it = m_loaders.find(metadata.type);
		if (it == m_loaders.end())
		{
			YUICY_CORE_WARN("[AssetLoader] No loader registered for asset type {} ({})",
				Utils::AssetTypeToString(metadata.type), metadata.filePath.string());
			return nullptr;
		}

		Ref<Asset> asset = it->second->Load(metadata, absolutePath);
		if (asset)
			asset->handle = metadata.handle;

		return asset;
	}

}
