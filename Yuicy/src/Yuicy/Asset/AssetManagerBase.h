#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Asset/Asset.h"

namespace Yuicy {

	// 资源查询与加载
	class AssetManagerBase
	{
	public:
		virtual ~AssetManagerBase() = default;

		// 首次访问时同步加载；未登记或加载失败时返回 nullptr
		virtual Ref<Asset> GetAsset(AssetHandle assetHandle) = 0;
		// 未登记时返回 AssetType::None
		virtual AssetType GetAssetType(AssetHandle assetHandle) const = 0;
		virtual bool IsAssetHandleValid(AssetHandle assetHandle) const = 0;
		virtual bool IsAssetLoaded(AssetHandle assetHandle) const = 0;
		// 为 asset 生成新的 Handle 并返回；资源只存在于内存中，不写入注册表
		virtual AssetHandle AddMemoryOnlyAsset(const Ref<Asset>& asset) = 0;

		template<typename T>
		Ref<T> GetAssetAs(AssetHandle assetHandle)
		{
			return std::dynamic_pointer_cast<T>(GetAsset(assetHandle));
		}
	};

}
