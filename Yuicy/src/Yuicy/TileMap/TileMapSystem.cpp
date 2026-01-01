#include "pch.h"
#include "Yuicy/Core/Log.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/TileMap/TileMapSystem.h"

namespace Yuicy {

	Ref<TileMap> TileMapSystem::LoadMap(const std::filesystem::path& filePath, Scene* scene, Ref<ITileMapBuilder> builder)
	{
		auto mapData = LoadMapData(filePath);
		if (!mapData)
		{
			YUICY_CORE_ERROR("TileMapSystem: Failed to load map from '{}'", filePath.string());
			return nullptr;
		}
		return CreateMap(mapData, scene, builder);
	}

	Ref<TileMap> TileMapSystem::CreateMap(Ref<ITileMapData> mapData, Scene* scene, Ref<ITileMapBuilder> builder)
	{
		if (!mapData || !scene)
		{
			YUICY_CORE_ERROR("TileMapSystem: Invalid map data or scene!");
			return nullptr;
		}
		Ref<TileMap> tileMap = nullptr;
		
		if (builder)
		{
			tileMap = CreateRef<TileMap>();
			tileMap->SetBuilder(builder);
			tileMap->Create(mapData, scene);
		}

		return tileMap;
	}

	Ref<ITileMapData> TileMapSystem::LoadMapData(const std::filesystem::path& filePath)
	{
		return TileMapLoader::Get().Load(filePath);
	}
}
