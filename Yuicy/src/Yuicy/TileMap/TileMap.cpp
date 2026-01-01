#include "pch.h"
#include "Yuicy/TileMap/TileMap.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	TileMap::~TileMap()
	{
		Destroy();
	}

	void TileMap::Create(Ref<ITileMapData> mapData, Scene* scene)
	{
		if (!mapData || !scene)
		{
			YUICY_CORE_ERROR("TileMap::Create - Invalid map data or scene!");
			return;
		}

		m_mapData = mapData;
		m_scene = scene;
		m_pixelsPerUnit = static_cast<float>(mapData->GetTileWidth());

		Build();

		YUICY_CORE_INFO("TileMap: Created '{}'", mapData->GetName());
	}

	void TileMap::Destroy()
	{
		if (!m_scene)
			return;

		for (auto& entity : m_entities)
		{
			if (entity)
				m_scene->DestroyEntity(entity);
		}
		m_entities.clear();
		m_mapData = nullptr;
		m_scene = nullptr;
	}

	void TileMap::OnUpdate(Timestep ts)
	{
	}

	void TileMap::Build()
	{
		if (!m_mapData || !m_scene)
			return;

		if (m_builder)
		{
			m_builder->Build(m_mapData.get(), m_scene, m_entities);
		}
	}

	glm::vec2 TileMap::GridToWorld(int32_t gridX, int32_t gridY) const
	{
		if (!m_mapData)
			return { 0.0f, 0.0f };

		auto worldSize = m_mapData->GetWorldSize();
		float worldX = static_cast<float>(gridX) + 0.5f;
		float worldY = static_cast<float>(static_cast<int32_t>(worldSize.y) - 1 - gridY) + 0.5f;
		return { worldX, worldY };
	}

	glm::ivec2 TileMap::WorldToGrid(float worldX, float worldY) const
	{
		if (!m_mapData)
			return { 0, 0 };

		auto worldSize = m_mapData->GetWorldSize();
		int32_t gridX = static_cast<int32_t>(std::floor(worldX));
		int32_t gridY = static_cast<int32_t>(worldSize.y) - 1 - static_cast<int32_t>(std::floor(worldY));
		return { gridX, gridY };
	}
}
