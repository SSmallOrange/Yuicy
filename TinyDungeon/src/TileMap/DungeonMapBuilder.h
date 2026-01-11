#pragma once

#include <Yuicy.h>
#include "DungeonTileMap.h"

namespace TinyDungeon {

	class DungeonMapBuilder : public Yuicy::ITileMapBuilder
	{
	public:
		DungeonMapBuilder() = default;
		~DungeonMapBuilder() override = default;

		void Build(Yuicy::ITileMapData* mapData, Yuicy::Scene* scene, std::vector<Yuicy::Entity>& outEntities) override;

	private:
		Yuicy::Ref<Yuicy::SubTexture2D> GetSubTextureBySliceId(TileMapData* data, const std::string& sliceId);
		glm::vec3 GridToWorld(TileMapData* data, int32_t gridX, int32_t gridY, int32_t gridZ);
	};
}
