#pragma once

#include <Yuicy.h>
#include "DungeonTileMap.h"
#include "Yuicy/TileMap/TileMapColliderMerger.h"

namespace TinyDungeon {

	class DungeonMapBuilder : public Yuicy::ITileMapBuilder
	{
	public:
		DungeonMapBuilder() = default;
		~DungeonMapBuilder() override = default;

		void Build(Yuicy::ITileMapData* mapData, Yuicy::Scene* scene, std::vector<Yuicy::Entity>& outEntities) override;

	private:
		Yuicy::Ref<Yuicy::SubTexture2D> GetSubTextureBySliceId(TileMapData* data, const std::string& sliceId);
		glm::vec3 GridToWorld(TileMapData* data, float gridX, float gridY, int32_t gridZ);

		// 构建2D碰撞网格
		std::vector<std::vector<bool>> BuildCollisionGrid(TileMapData* data) const;

		// 为合并后的碰撞矩形列表创建实体
		void BuildMergedColliderEntities(TileMapData* data, Yuicy::Scene* scene, const std::vector<Yuicy::MergedColliderRect>& mergedColliders,
			std::vector<Yuicy::Entity>& outEntities);

		// 为单个 tile 创建独立碰撞体
		void BuildIndividualCollider(Yuicy::Entity entity, const TileInstance& tile);
	};
}
