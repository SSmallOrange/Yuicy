#include "pch.h"
#include "Yuicy/Scene/SceneAssetLoaders.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy {

	namespace {

		class SceneAssetLoader : public AssetLoader
		{
		public:
			Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const override
			{
				// 只创建空场景，不读取文件内容
				return CreateRef<Scene>();
			}
		};

	}

	void RegisterSceneAssetLoaders(AssetLoaderRegistry& registry)
	{
		registry.Register(AssetType::Scene, CreateScope<SceneAssetLoader>());
	}

}
