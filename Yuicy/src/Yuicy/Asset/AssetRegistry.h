#pragma once

#include "AssetMetadata.h"

#include <unordered_map>

namespace Yuicy {

	class AssetRegistry
	{
	public:
		const AssetMetadata& Get(const AssetHandle handle) const;
		void Set(const AssetHandle handle, const AssetMetadata& metadata);

		size_t Count() const { return m_assetRegistry.size(); }
		bool Contains(const AssetHandle handle) const;
		size_t Remove(const AssetHandle handle);
		void Clear();

		auto begin() { return m_assetRegistry.begin(); }
		auto end() { return m_assetRegistry.end(); }
		auto begin() const { return m_assetRegistry.cbegin(); }
		auto end() const { return m_assetRegistry.cend(); }
	private:
		std::unordered_map<AssetHandle, AssetMetadata> m_assetRegistry;
	};

}
