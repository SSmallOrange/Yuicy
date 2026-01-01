#pragma once

#include "Yuicy/Core/Timestep.h"
#include "Yuicy/TileMap/TileMapCommon.h"

namespace Yuicy {

	class Scene;
	class Entity;

	class TileMap
	{
	public:
		TileMap() = default;
		virtual ~TileMap();

		void Create(Ref<ITileMapData> mapData, Scene* scene);
		void Destroy();
		virtual void OnUpdate(Timestep ts);

		Ref<ITileMapData> GetMapData() const { return m_mapData; }
		Scene* GetScene() const { return m_scene; }
		const std::vector<Entity>& GetEntities() const { return m_entities; }

		glm::vec2 GridToWorld(int32_t gridX, int32_t gridY) const;
		glm::ivec2 WorldToGrid(float worldX, float worldY) const;

		// 设置自定义构建器
		void SetBuilder(Ref<ITileMapBuilder> builder) { m_builder = builder; }

	protected:
		virtual void Build();

	protected:
		Ref<ITileMapData> m_mapData = nullptr;
		Scene* m_scene = nullptr;
		std::vector<Entity> m_entities;
		Ref<ITileMapBuilder> m_builder = nullptr;
		float m_pixelsPerUnit = 16.0f;
	};

	struct TileMapComponent
	{
		Ref<TileMap> tileMap = nullptr;
		std::string mapFilePath;

		TileMapComponent() = default;
		TileMapComponent(const TileMapComponent&) = default;
	};

}