#pragma once

#include "Yuicy/Scene/Components.h"

#include <array>
#include <vector>

namespace Yuicy {

	struct TilemapColliderShape
	{
		GridPosition cell;
		std::array<glm::vec2, 4> points = {};
		TileColliderType colliderType = TileColliderType::Grid;
	};

	struct TilemapColliderRect
	{
		GridPosition origin;
		int width = 1;
		int height = 1;
		TileColliderType colliderType = TileColliderType::Grid;
	};

	class TilemapColliderGeometry
	{
	public:
		static std::vector<TilemapColliderShape> BuildGridShapes(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider);

		static std::vector<TilemapColliderShape> BuildMergedGridShapes(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider);

	private:
		static std::vector<TilemapColliderRect> BuildMergedGridRects(
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider);
	};

}
