#pragma once

#include "Yuicy/Asset/AssetMetadata.h"

namespace Yuicy {

	class AssetSerializer
	{
	public:
		virtual ~AssetSerializer() = default;
		
		// asset 写回
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		// asset 加载
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
	};

	class AssetImporter
	{
	public:
		static void Init();
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset);
	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_serializers;
	};

}
