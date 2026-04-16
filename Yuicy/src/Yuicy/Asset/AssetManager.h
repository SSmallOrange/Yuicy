#pragma once

#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Asset/AssetTypes.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Asset/EditorAssetManager.h"

#include <unordered_set>

namespace Yuicy {

	class AssetManager
	{
	public:
		static bool IsAssetHandleValid(AssetHandle assetHandle)
		{
			return Project::GetEditorAssetManager()->IsAssetHandleValid(assetHandle);
		}

		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			Ref<Asset> asset = Project::GetEditorAssetManager()->GetAsset(assetHandle);
			return std::dynamic_pointer_cast<T>(asset);
		}

		static AssetType GetAssetType(AssetHandle assetHandle)
		{
			return Project::GetEditorAssetManager()->GetAssetType(assetHandle);
		}

		static bool ReloadData(AssetHandle assetHandle)
		{
			return Project::GetEditorAssetManager()->ReloadData(assetHandle);
		}

		static void RemoveAsset(AssetHandle assetHandle)
		{
			Project::GetEditorAssetManager()->RemoveAsset(assetHandle);
		}

		template<typename TAsset>
		static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset)
		{
			static_assert(std::is_base_of<Asset, TAsset>::value, "AddMemoryOnlyAsset only works for types derived from Asset");
			asset->handle = AssetHandle(); // 生成新 UUID
			Project::GetEditorAssetManager()->AddMemoryOnlyAsset(asset);
			return asset->handle;
		}

		template<typename T>
		static std::unordered_set<AssetHandle> GetAllAssetsWithType()
		{
			return Project::GetEditorAssetManager()->GetAllAssetsWithType(T::GetStaticType());
		}
	};

}
