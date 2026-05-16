#pragma once

#include "Yuicy/Scene/Components.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <glm/glm.hpp>

namespace Yuicy {

	struct GridLayoutUtility
	{
		static GridPosition LocalToCell(glm::vec2 local, const GridComponent& grid);
		static glm::vec2 CellToLocal(const GridPosition& cell, const GridComponent& grid);
		static GridPosition WorldToCell(glm::vec2 world, const glm::mat4& gridWorldTransform, const GridComponent& grid);
		static glm::vec2 CellToWorld(const GridPosition& cell, const glm::mat4& gridWorldTransform, const GridComponent& grid);

		static glm::vec2 GetCellStride(const GridComponent& grid);
	};

}
