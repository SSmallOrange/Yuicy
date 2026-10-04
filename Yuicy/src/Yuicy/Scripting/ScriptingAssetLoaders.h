#pragma once

namespace Yuicy {

	class AssetLoaderRegistry;

	// 加载 LuaScript 前需要先调用 LuaScriptEngine::Init
	void RegisterScriptingAssetLoaders(AssetLoaderRegistry& registry);

}
