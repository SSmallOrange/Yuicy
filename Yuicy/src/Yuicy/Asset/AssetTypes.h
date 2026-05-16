#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Assert.h"

#include <string_view>

namespace Yuicy {

	enum class AssetFlag : uint16_t
	{
		None = 0,
		Missing = BIT(0),
		Invalid = BIT(1),
	};

	enum class AssetType : uint16_t
	{
		None = 0,
		Scene,		// .yui
		Texture,	// .png, .jpg, .jpeg
		Font,       // .ttf, .otf
		Shader,     // .glsl
		LuaScript,	// .lua
		Sprite,     // .ysprite
		Tile,       // .ytile
		TilePalette // .ypalette
		// ...
	};

	namespace Utils {

		inline AssetType AssetTypeFromString(std::string_view assetType)
		{
			if (assetType == "None")       return AssetType::None;
			if (assetType == "Scene")      return AssetType::Scene;
			if (assetType == "Texture")    return AssetType::Texture;
			if (assetType == "Font")       return AssetType::Font;
			if (assetType == "Shader")     return AssetType::Shader;
			if (assetType == "LuaScript")  return AssetType::LuaScript;
			if (assetType == "Sprite")     return AssetType::Sprite;
			if (assetType == "Tile")       return AssetType::Tile;
			if (assetType == "TilePalette") return AssetType::TilePalette;

			return AssetType::None;
		}

		inline const char* AssetTypeToString(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::None:       return "None";
				case AssetType::Scene:      return "Scene";
				case AssetType::Texture:    return "Texture";
				case AssetType::Font:       return "Font";
				case AssetType::Shader:     return "Shader";
				case AssetType::LuaScript:  return "LuaScript";
				case AssetType::Sprite:     return "Sprite";
				case AssetType::Tile:       return "Tile";
				case AssetType::TilePalette: return "TilePalette";
			}

			YUICY_CORE_ASSERT(false, "Unknown Asset Type");
			return "None";
		}

	}
}
