#include "pch.h"
#include "TilemapRenderer2D.h"

#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Renderer/SubTexture.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Sprite/SpriteAsset.h"
#include "Yuicy/Tilemap/GridLayoutUtility.h"
#include "Yuicy/Tilemap/Tile.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Yuicy {

	void TilemapRenderer2D::DrawTilemap(const glm::mat4& transform, const GridComponent& grid, const TilemapComponent& tilemap,
			const TilemapRendererComponent& renderer, const TilemapRenderContext& context, int entityId)
	{
		(void)renderer;
		(void)context;

		if (tilemap.m_cells.empty())
			return;

		std::vector<GridPosition> sortedCells;
		sortedCells.reserve(tilemap.m_cells.size());
		for (const auto& [position, cell] : tilemap.m_cells)
			sortedCells.push_back(position);

		std::ranges::sort(sortedCells, [](const GridPosition& a, const GridPosition& b) {
			return a < b;
		});

		for (const GridPosition& position : sortedCells)
		{
			const TileCell* cell = tilemap.GetTile(position);
			if (!cell || cell->m_tileHandle == 0)
				continue;

			Ref<TileAsset> tileAsset = nullptr;
			glm::vec4 tintColor = tilemap.m_color * cell->m_color;
			Ref<SubTexture2D> spriteSubTexture = nullptr;

			if (Project::GetActive())
				tileAsset = AssetManager::GetAsset<TileAsset>(cell->m_tileHandle);

			if (tileAsset)
			{
				tintColor *= tileAsset->m_color;

				if (tileAsset->m_spriteHandle != 0)
				{
					Ref<SpriteAsset> spriteAsset = AssetManager::GetAsset<SpriteAsset>(tileAsset->m_spriteHandle);
					if (spriteAsset && spriteAsset->m_textureHandle != 0)
					{
						Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(spriteAsset->m_textureHandle);
						if (texture)
							spriteSubTexture = CreateRef<SubTexture2D>(texture, spriteAsset->m_uvMin, spriteAsset->m_uvMax);
					}
				}
			}

			const glm::vec2 cellOrigin = GridLayoutUtility::CellToLocal(position, grid);
			const glm::vec3 anchorOffset = {
				grid.m_cellSize.x * tilemap.m_tileAnchor.x,
				grid.m_cellSize.y * tilemap.m_tileAnchor.y,
				tilemap.m_tileAnchor.z
			};

			glm::mat4 tileTransform = transform
				* glm::translate(glm::mat4(1.0f), glm::vec3(cellOrigin, 0.0f) + anchorOffset)
				* glm::scale(glm::mat4(1.0f), glm::vec3(grid.m_cellSize, 1.0f));

			if (spriteSubTexture)
				Renderer2D::DrawSprite(tileTransform, spriteSubTexture, 1.0f, tintColor, false, false, entityId);
			else
				Renderer2D::DrawQuad(tileTransform, tintColor, entityId);
		}
	}

}
