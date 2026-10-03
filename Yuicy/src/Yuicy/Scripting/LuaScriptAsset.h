#pragma once

#include "Yuicy/Asset/Asset.h"

namespace Yuicy {

	// 只用于在 EditorAssetManager 中占位，编译结果由 LuaScriptEngine 按文件路径缓存
	class LuaScriptAsset : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::LuaScript; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}
