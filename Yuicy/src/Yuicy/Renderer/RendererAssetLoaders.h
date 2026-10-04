#pragma once

namespace Yuicy {

	class AssetLoaderRegistry;

	// 加载 Texture、Shader 时会创建 GPU 资源，需要有效的图形上下文
	void RegisterRendererAssetLoaders(AssetLoaderRegistry& registry);

}
