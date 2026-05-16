#pragma once

#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <glm/glm.hpp>

namespace Yuicy {

	struct TileAsset : public Asset
	{
		AssetHandle m_spriteHandle = 0;
		glm::vec4 m_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		TileColliderType m_colliderType = TileColliderType::None;
		uint32_t m_flags = 0;

		static AssetType GetStaticType() { return AssetType::Tile; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}
