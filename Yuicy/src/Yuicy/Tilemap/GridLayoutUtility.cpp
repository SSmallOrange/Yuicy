#include "pch.h"
#include "GridLayoutUtility.h"

#include <cmath>

#include <glm/gtc/matrix_inverse.hpp>

namespace Yuicy {

	namespace {

		float SafeStride(float value)
		{
			if (std::abs(value) <= 0.0001f)
				return 1.0f;
			return value;
		}

	}

	glm::vec2 GridLayoutUtility::GetCellStride(const GridComponent& grid)
	{
		glm::vec2 stride = grid.m_cellSize + grid.m_cellGap;
		return { SafeStride(stride.x), SafeStride(stride.y) };
	}

	GridPosition GridLayoutUtility::LocalToCell(glm::vec2 local, const GridComponent& grid)
	{
		const glm::vec2 stride = GetCellStride(grid);

		return {
			(int)std::floor(local.x / stride.x),
			(int)std::floor(local.y / stride.y),
			0
		};
	}

	glm::vec2 GridLayoutUtility::CellToLocal(const GridPosition& cell, const GridComponent& grid)
	{
		const glm::vec2 stride = GetCellStride(grid);
		return {
			(float)cell.m_x * stride.x,
			(float)cell.m_y * stride.y
		};
	}

	GridPosition GridLayoutUtility::WorldToCell(glm::vec2 world, const glm::mat4& gridWorldTransform, const GridComponent& grid)
	{
		glm::vec4 local = glm::inverse(gridWorldTransform) * glm::vec4(world.x, world.y, 0.0f, 1.0f);
		return LocalToCell({ local.x, local.y }, grid);
	}

	glm::vec2 GridLayoutUtility::CellToWorld(const GridPosition& cell, const glm::mat4& gridWorldTransform, const GridComponent& grid)
	{
		glm::vec2 local = CellToLocal(cell, grid);
		glm::vec4 world = gridWorldTransform * glm::vec4(local.x, local.y, 0.0f, 1.0f);
		return { world.x, world.y };
	}

}
