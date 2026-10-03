#pragma once

namespace Yuicy {

	struct SortingLayerConfig;
	struct SpriteRendererComponent;

	// a 应先于 b 绘制（被 b 覆盖）时返回 true：先比较所在排序层的 Order，再比较 SortingOrder。
	// 满足严格弱序，可直接用作 std::sort 的比较函数
	bool IsSpriteDrawnBefore(const SpriteRendererComponent& a, const SpriteRendererComponent& b, const SortingLayerConfig& sortingLayers);

}
