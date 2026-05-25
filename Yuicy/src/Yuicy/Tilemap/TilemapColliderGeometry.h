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

	class TilemapColliderGeometry
	{
	public:
		static std::vector<TilemapColliderShape> BuildGridShapes(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider);
	};

}
