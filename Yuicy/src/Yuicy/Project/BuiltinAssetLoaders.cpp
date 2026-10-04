#include "pch.h"
#include "Yuicy/Project/BuiltinAssetLoaders.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Renderer/RendererAssetLoaders.h"
#include "Yuicy/Scene/SceneAssetLoaders.h"
#include "Yuicy/Scripting/ScriptingAssetLoaders.h"

namespace Yuicy {

	Ref<const AssetLoaderRegistry> CreateBuiltinAssetLoaders()
	{
		Ref<AssetLoaderRegistry> registry = CreateRef<AssetLoaderRegistry>();
		RegisterRendererAssetLoaders(*registry);
		RegisterScriptingAssetLoaders(*registry);
		RegisterSceneAssetLoaders(*registry);
		// TODO: 注册 Font 的 Loader（实现字体加载时）
		return registry;
	}

}
