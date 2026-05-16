#pragma once

#include "Yuicy/Scene/Components.h"

#include <glm/glm.hpp>

namespace Yuicy {

	struct TilemapRenderContext
	{
	};

	class TilemapRenderer2D
	{
	public:
		static void DrawTilemap(const glm::mat4& transform, const GridComponent& grid, const TilemapComponent& tilemap,
			const TilemapRendererComponent& renderer, const TilemapRenderContext& context, int entityId);
	};

}
