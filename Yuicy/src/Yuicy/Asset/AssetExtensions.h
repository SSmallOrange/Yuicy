#pragma once

#include <unordered_map>

#include "AssetTypes.h"

namespace Yuicy {

	inline static std::unordered_map<std::string, AssetType> s_assetExtensionMap =
	{
		// Yuicy types
		{ ".yui",  AssetType::Scene },

		// Textures
		{ ".png",  AssetType::Texture },
		{ ".jpg",  AssetType::Texture },
		{ ".jpeg", AssetType::Texture },

		// Fonts
		{ ".ttf",  AssetType::Font },
		{ ".otf",  AssetType::Font },

		// Shaders
		{ ".glsl", AssetType::Shader },

		// Lua Scripts
		{ ".lua",  AssetType::LuaScript },

		// Tilemap assets
		{ ".ysprite", AssetType::Sprite },
		{ ".ytile",   AssetType::Tile },
	};

}
