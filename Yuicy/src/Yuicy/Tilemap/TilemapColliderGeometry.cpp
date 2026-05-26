#include "pch.h"
#include "TilemapColliderGeometry.h"

#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Tilemap/GridLayoutUtility.h"
#include "Yuicy/Tilemap/Tile.h"

#include <algorithm>
#include <unordered_set>

#include <glm/gtc/matrix_transform.hpp>

namespace Yuicy {

	namespace {

		struct SolidGridCell
		{
			GridPosition position;
			TileColliderType colliderType = TileColliderType::Grid;
		};

		void WarnMissingTileAssetOnce(AssetHandle tileHandle)
		{
			static std::unordered_set<uint64_t> warnedTileHandles;

			uint64_t key = (uint64_t)tileHandle;
			if (!warnedTileHandles.insert(key).second)
				return;

			YUICY_CORE_WARN("TilemapColliderGeometry skipped tile asset because it failed to load. Handle: {}", key);
		}

		TileColliderType ResolveColliderType(const TileCell& cell, const TilemapCollider2DComponent& collider)
		{
			(void)collider;

			if (cell.m_tileHandle == 0 || !Project::GetEditorAssetManager())
				return TileColliderType::None;

			Ref<TileAsset> tileAsset = AssetManager::GetAsset<TileAsset>(cell.m_tileHandle);
			if (!tileAsset)
			{
				WarnMissingTileAssetOnce(cell.m_tileHandle);
				return TileColliderType::None;
			}

			// TileAsset 的 None 表示显式无碰撞，不被 defaultColliderType 覆盖
			return tileAsset->m_colliderType;
		}

		std::vector<GridPosition> CollectSortedTilePositions(const TilemapComponent& tilemap)
		{
			std::vector<GridPosition> sortedPositions;
			sortedPositions.reserve(tilemap.m_cells.size());

			for (const auto& [position, cell] : tilemap.m_cells)
			{
				if (cell.m_tileHandle != 0)
					sortedPositions.push_back(position);
			}

			std::ranges::sort(sortedPositions, [](const GridPosition& a, const GridPosition& b) {
				return a < b;
			});

			return sortedPositions;
		}

		std::vector<SolidGridCell> CollectSolidGridCells(
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider)
		{
			std::vector<SolidGridCell> solidCells;

			if (grid.m_layout != GridLayout::Rectangular || tilemap.m_cells.empty())
				return solidCells;

			std::vector<GridPosition> sortedPositions = CollectSortedTilePositions(tilemap);
			solidCells.reserve(sortedPositions.size());

			for (const GridPosition& position : sortedPositions)
			{
				const TileCell* cell = tilemap.GetTile(position);
				if (!cell || cell->m_tileHandle == 0)
					continue;

				const TileColliderType colliderType = ResolveColliderType(*cell, collider);
				if (colliderType != TileColliderType::Grid)
					continue;

				solidCells.push_back({ position, colliderType });
			}

			return solidCells;
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

		TilemapColliderShape BuildGridShapeFromRect(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapColliderRect& rect)
		{
			const glm::vec2 origin = GridLayoutUtility::CellToLocal(rect.origin, grid);
			const glm::vec2 anchor = { tilemap.m_tileAnchor.x, tilemap.m_tileAnchor.y };
			const glm::vec2 localMin = origin + (anchor - glm::vec2(0.5f)) * grid.m_cellSize;
			const glm::vec2 localMax = origin + (glm::vec2(rect.width - 1, rect.height - 1) + anchor + glm::vec2(0.5f)) * grid.m_cellSize;

			const std::array<glm::vec4, 4> localCorners = {
				glm::vec4(localMin.x, localMin.y, tilemap.m_tileAnchor.z, 1.0f),
				glm::vec4(localMax.x, localMin.y, tilemap.m_tileAnchor.z, 1.0f),
				glm::vec4(localMax.x, localMax.y, tilemap.m_tileAnchor.z, 1.0f),
				glm::vec4(localMin.x, localMax.y, tilemap.m_tileAnchor.z, 1.0f)
			};

			TilemapColliderShape shape;
			shape.cell = rect.origin;
			shape.colliderType = rect.colliderType;

			for (size_t i = 0; i < localCorners.size(); i++)
			{
				const glm::vec4 worldPoint = tilemapWorldTransform * localCorners[i];
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
		std::vector<SolidGridCell> solidCells = CollectSolidGridCells(grid, tilemap, collider);
		shapes.reserve(solidCells.size());

		for (const SolidGridCell& solidCell : solidCells)
			shapes.push_back(BuildGridShape(tilemapWorldTransform, grid, tilemap, solidCell.position, solidCell.colliderType));

		return shapes;
	}

	std::vector<TilemapColliderShape> TilemapColliderGeometry::BuildMergedGridShapes(
		const glm::mat4& tilemapWorldTransform,
		const GridComponent& grid,
		const TilemapComponent& tilemap,
		const TilemapCollider2DComponent& collider)
	{
		std::vector<TilemapColliderRect> rects = BuildMergedGridRects(grid, tilemap, collider);
		std::vector<TilemapColliderShape> shapes;
		shapes.reserve(rects.size());

		for (const TilemapColliderRect& rect : rects)
			shapes.push_back(BuildGridShapeFromRect(tilemapWorldTransform, grid, tilemap, rect));

		return shapes;
	}

	std::vector<TilemapColliderRect> TilemapColliderGeometry::BuildMergedGridRects(
		const GridComponent& grid,
		const TilemapComponent& tilemap,
		const TilemapCollider2DComponent& collider)
	{
		std::vector<TilemapColliderRect> rects;
		std::vector<SolidGridCell> solidCells = CollectSolidGridCells(grid, tilemap, collider);
		rects.reserve(solidCells.size());

		for (const SolidGridCell& solidCell : solidCells)
		{
			TilemapColliderRect rect;
			rect.origin = solidCell.position;
			rect.colliderType = solidCell.colliderType;
			rects.push_back(rect);
		}

		return rects;
	}

}
