#pragma once

#include "Yuicy/Core/Base.h"

#include <glm/glm.hpp>
#include <vector>

namespace Yuicy {

	class Scene;
	class Entity;
	
	struct ITileMapData
	{
		std::string filePath;

		virtual ~ITileMapData() = default;
		virtual glm::vec2 GetWorldSize() const = 0;
		virtual int32_t GetTileWidth() const = 0;
		virtual int32_t GetTileHeight() const = 0;
		virtual const std::string& GetName() const = 0;
	};

	// 自定义瓦片地图构建
	class ITileMapBuilder
	{
	public:
		virtual ~ITileMapBuilder() = default;
		virtual void Build(ITileMapData* mapData, Scene* scene, std::vector<Entity>& outEntities) = 0;
	};
}
