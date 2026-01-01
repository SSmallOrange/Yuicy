#pragma once

#include "Yuicy/TileMap/TileMap.h"
#include "Yuicy/TileMap/TileMapLoader.h"

namespace Yuicy {

	class TileMapSystem
	{
	public:
		static Ref<TileMap> LoadMap(const std::filesystem::path& filePath, Scene* scene, Ref<ITileMapBuilder> builder);

		static Ref<TileMap> CreateMap(Ref<ITileMapData> mapData, Scene* scene, Ref<ITileMapBuilder> builder);

		static Ref<ITileMapData> LoadMapData(const std::filesystem::path& filePath);

		template<typename T>
		static Ref<T> LoadMapDataAs(const std::filesystem::path& filePath)
		{
			return TileMapLoader::Get().LoadAs<T>(filePath);
		}

		static TileMapLoader& GetLoader() { return TileMapLoader::Get(); }
	};

}