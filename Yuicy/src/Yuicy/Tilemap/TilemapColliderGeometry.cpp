#include "pch.h"
#include "TilemapColliderGeometry.h"

#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Tilemap/GridLayoutUtility.h"
#include "Yuicy/Tilemap/Tile.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

namespace Yuicy {

	namespace {

		TileColliderType ResolveColliderType(const TileCell& cell, const TilemapCollider2DComponent& collider)
		{
			(void)collider;

			if (cell.m_tileHandle == 0 || !Project::GetEditorAssetManager())
				return TileColliderType::None;

			Ref<TileAsset> tileAsset = AssetManager::GetAsset<TileAsset>(cell.m_tileHandle);
			if (!tileAsset)
				return TileColliderType::None;

			// TileAsset 的 None 表示显式无碰撞，不被 defaultColliderType 覆盖
			return tileAsset->m_colliderType;
		}

		TilemapColliderShape BuildGridShape(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const GridPosition& position,
			TileColliderType colliderType)
		{
			const glm::vec2 cellOrigin = GridLayoutUtility::CellToLocal(position, grid);
			const glm::vec3 anchorOffset = {
				grid.m_cellSize.x * tilemap.m_tileAnchor.x,
				grid.m_cellSize.y * tilemap.m_tileAnchor.y,
				tilemap.m_tileAnchor.z
			};

			const glm::mat4 tileTransform = tilemapWorldTransform
				* glm::translate(glm::mat4(1.0f), glm::vec3(cellOrigin, 0.0f) + anchorOffset)
				* glm::scale(glm::mat4(1.0f), glm::vec3(grid.m_cellSize, 1.0f));

			const std::array<glm::vec4, 4> localCorners = {
				glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
				glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f),
				glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f),
				glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f)
			};

			TilemapColliderShape shape;
			shape.cell = position;
			shape.colliderType = colliderType;

			for (size_t i = 0; i < localCorners.size(); i++)
			{
				const glm::vec4 worldPoint = tileTransform * localCorners[i];
				shape.points[i] = { worldPoint.x, worldPoint.y };
			}

			return shape;
		}

	}

	std::vector<TilemapColliderShape> TilemapColliderGeometry::BuildGridShapes(
		const glm::mat4& tilemapWorldTransform,
		const GridComponent& grid,
		const TilemapComponent& tilemap,
		const TilemapCollider2DComponent& collider)
	{
		std::vector<TilemapColliderShape> shapes;

		if (grid.m_layout != GridLayout::Rectangular || tilemap.m_cells.empty())
			return shapes;

		std::vector<GridPosition> sortedCells;
		sortedCells.reserve(tilemap.m_cells.size());
		for (const auto& [position, cell] : tilemap.m_cells)
		{
			if (cell.m_tileHandle != 0)
				sortedCells.push_back(position);
		}

		std::ranges::sort(sortedCells, [](const GridPosition& a, const GridPosition& b) {
			return a < b;
		});
		shapes.reserve(sortedCells.size());

		for (const GridPosition& position : sortedCells)
		{
			const TileCell* cell = tilemap.GetTile(position);
			if (!cell || cell->m_tileHandle == 0)
				continue;

			const TileColliderType colliderType = ResolveColliderType(*cell, collider);
			if (colliderType != TileColliderType::Grid)
				continue;

			shapes.push_back(BuildGridShape(tilemapWorldTransform, grid, tilemap, position, colliderType));
		}

		return shapes;
	}

}
