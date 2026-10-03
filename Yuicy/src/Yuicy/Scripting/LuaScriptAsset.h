#pragma once

#include "Yuicy/Asset/Asset.h"

#include <filesystem>

namespace Yuicy {

	// 编译结果由 LuaScriptEngine 按文件路径缓存，资源本身只记录这个路径
	class LuaScriptAsset : public Asset
	{
	public:
		explicit LuaScriptAsset(std::filesystem::path filePath)
			: m_FilePath(std::move(filePath))
		{
		}

		static AssetType GetStaticType() { return AssetType::LuaScript; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		// 加载时编译的脚本文件路径，用它调用 LuaScriptEngine 会命中同一份缓存
		const std::filesystem::path& GetFilePath() const { return m_FilePath; }

	private:
		std::filesystem::path m_FilePath;
	};

}
