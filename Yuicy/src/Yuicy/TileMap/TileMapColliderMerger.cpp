#include "pch.h"
#include "Yuicy/TileMap/TileMapColliderMerger.h"

#include <numeric>

namespace Yuicy {

	std::vector<MergedColliderRect> TileMapColliderMerger::Merge(const std::vector<std::vector<bool>>& collisionGrid)
	{
		std::vector<MergedColliderRect> result;

		if (collisionGrid.empty())
			return result;

		const int32_t mapHeight = static_cast<int32_t>(collisionGrid.size());
		const int32_t mapWidth  = static_cast<int32_t>(collisionGrid[0].size());

		// 统计碰撞格子总数
		int32_t totalCollisionCells = 0;

		// 已访问标记网格
		std::vector<std::vector<bool>> visited(mapHeight, std::vector<bool>(mapWidth, false));

		for (int32_t y = 0; y < mapHeight; ++y)
		{
			for (int32_t x = 0; x < mapWidth; ++x)
			{
				// 跳过非碰撞格子或已处理的格子
				if (!collisionGrid[y][x] || visited[y][x])
					continue;

				// 向右扩展
				int32_t maxWidth = ScanMaxWidth(collisionGrid, visited, x, y, mapWidth);

				// 向下扩展
				int32_t maxHeight = 1;
				while (y + maxHeight < mapHeight)
				{
					if (!IsRowRangeValid(collisionGrid, visited, y + maxHeight, x, maxWidth))
						break;
					++maxHeight;
				}

				// 3. 标记覆盖区域为已访问
				MarkVisited(visited, x, y, maxWidth, maxHeight);

				// 4. 记录合并后的矩形
				result.push_back({ x, y, maxWidth, maxHeight });
				totalCollisionCells += maxWidth * maxHeight;
			}
		}

		YUICY_CORE_INFO("TileMapColliderMerger: Merged {} collision tiles into {} colliders (reduction: {:.0f}%)",
			totalCollisionCells,
			result.size(),
			result.empty() ? 0.0 : (1.0 - static_cast<double>(result.size()) / totalCollisionCells) * 100.0);

		return result;
	}

	int32_t TileMapColliderMerger::ScanMaxWidth(const std::vector<std::vector<bool>>& grid, const std::vector<std::vector<bool>>& visited,
		int32_t startX, int32_t startY, int32_t mapWidth)
	{
		int32_t width = 0;
		// 探索最右Index
		while (startX + width < mapWidth && grid[startY][startX + width] && !visited[startY][startX + width])
		{
			++width;
		}
		return width;
	}

	bool TileMapColliderMerger::IsRowRangeValid(const std::vector<std::vector<bool>>& grid, const std::vector<std::vector<bool>>& visited,
		int32_t row, int32_t startX, int32_t targetWidth)
	{
		for (int32_t dx = 0; dx < targetWidth; ++dx)
		{
			if (!grid[row][startX + dx] || visited[row][startX + dx])
				return false;
		}
		return true;
	}

	void TileMapColliderMerger::MarkVisited(std::vector<std::vector<bool>>& visited,
		int32_t startX, int32_t startY, int32_t width, int32_t height)
	{
		for (int32_t dy = 0; dy < height; ++dy)
		{
			for (int32_t dx = 0; dx < width; ++dx)
			{
				visited[startY + dy][startX + dx] = true;
			}
		}
	}
}
