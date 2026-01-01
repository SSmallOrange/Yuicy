#include "DungeonTileMap.h"

namespace TinyDungeon {

	Yuicy::Ref<Yuicy::SubTexture2D> TileSet::GetSubTextureBySliceId(const std::string& sliceId) const
	{
		for (const auto& slice : slices)
		{
			if (slice.id == sliceId)
				return slice.subTexture;
		}
		return nullptr;
	}

	TileSet* TileMapData::FindTileSetByName(const std::string& name)
	{
		for (auto& tileset : tilesets)
		{
			if (tileset.name == name)
				return &tileset;
		}
		return nullptr;
	}

	SliceDef* TileMapData::FindSliceById(const std::string& sliceId)
	{
		for (auto& slice : slices)
		{
			if (slice.id == sliceId)
				return &slice;
		}
		return nullptr;
	}

	TilesetSlice* TileMapData::FindTilesetSliceById(const std::string& sliceId)
	{
		for (auto& tileset : tilesets)
		{
			for (auto& slice : tileset.slices)
			{
				if (slice.id == sliceId)
					return &slice;
			}
		}
		return nullptr;
	}
}
