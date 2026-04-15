#include "pch.h"
#include "AssetRegistry.h"

namespace Yuicy {

	const AssetMetadata& AssetRegistry::Get(const AssetHandle handle) const
	{
		YUICY_CORE_ASSERT(m_assetRegistry.find(handle) != m_assetRegistry.end());
		return m_assetRegistry.at(handle);
	}

	void AssetRegistry::Set(const AssetHandle handle, const AssetMetadata& metadata)
	{
		YUICY_CORE_ASSERT(metadata.handle == handle);
		YUICY_CORE_ASSERT(handle != 0);
		m_assetRegistry[handle] = metadata;
	}

	bool AssetRegistry::Contains(const AssetHandle handle) const
	{
		return m_assetRegistry.find(handle) != m_assetRegistry.end();
	}

	size_t AssetRegistry::Remove(const AssetHandle handle)
	{
		return m_assetRegistry.erase(handle);
	}

	void AssetRegistry::Clear()
	{
		m_assetRegistry.clear();
	}

}
