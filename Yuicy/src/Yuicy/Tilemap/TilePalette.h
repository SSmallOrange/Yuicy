#pragma once

#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace Yuicy {

	struct TilePaletteAsset : public Asset
	{
		std::string m_name;
		GridLayout m_layout = GridLayout::Rectangular;
		glm::vec2 m_cellSize = { 1.0f, 1.0f };
		glm::vec2 m_cellGap = { 0.0f, 0.0f };
		std::unordered_map<GridPosition, AssetHandle, GridPositionHash> m_cells;

		bool HasTile(const GridPosition& position) const
		{
			return m_cells.find(position) != m_cells.end();
		}

		AssetHandle GetTile(const GridPosition& position) const
		{
			if (const auto it = m_cells.find(position); it != m_cells.end())
				return it->second;

			return 0;
		}

		void SetTile(const GridPosition& position, AssetHandle tileHandle)
		{
			if (tileHandle == 0)
			{
				EraseTile(position);
				return;
			}

			m_cells[position] = tileHandle;
		}

		void EraseTile(const GridPosition& position)
		{
			m_cells.erase(position);
		}

		void Clear()
		{
			m_cells.clear();
		}

		static AssetType GetStaticType() { return AssetType::TilePalette; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}
