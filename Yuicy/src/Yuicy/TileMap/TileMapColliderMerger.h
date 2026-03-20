#pragma once

#include <cstdint>
#include <vector>

namespace Yuicy {

	struct MergedColliderRect
	{
		int32_t x = 0;
		int32_t y = 0;
		int32_t width = 1;
		int32_t height = 1;

		int32_t Area() const { return width * height; }
	};

	class TileMapColliderMerger
	{
	public:
		// 合并碰撞网格
		static std::vector<MergedColliderRect> Merge(const std::vector<std::vector<bool>>& collisionGrid);

	private:
		// 扫描某个起点向右可扩展的最大宽度
		static int32_t ScanMaxWidth(const std::vector<std::vector<bool>>& grid, const std::vector<std::vector<bool>>& visited,
			int32_t startX, int32_t startY, int32_t mapWidth);

		// 检查某一行中从 startX 开始的 targetWidth 个格子是否全部为碰撞且未访问
		static bool IsRowRangeValid(const std::vector<std::vector<bool>>& grid, const std::vector<std::vector<bool>>& visited,
			int32_t row, int32_t startX, int32_t targetWidth);

		// 将矩形区域标记为已访问
		static void MarkVisited(std::vector<std::vector<bool>>& visited, int32_t startX, int32_t startY,
			int32_t width, int32_t height);
	};
}
