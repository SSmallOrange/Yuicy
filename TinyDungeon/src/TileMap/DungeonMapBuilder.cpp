#include <Yuicy.h>
#include "DungeonMapBuilder.h"

namespace TinyDungeon {

	void DungeonMapBuilder::Build(Yuicy::ITileMapData* mapData, Yuicy::Scene* scene, std::vector<Yuicy::Entity>& outEntities)
	{
		TileMapData* data = dynamic_cast<TileMapData*>(mapData);
		if (!data)
		{
			YUICY_CORE_ERROR("DungeonMapBuilder: Invalid map data type");
			return;
		}

		for (const auto& layer : data->layers)
		{
			if (!layer.visible)
				continue;

			for (const auto& tile : layer.tiles)
			{
				auto subTexture = GetSubTextureBySliceId(data, tile.sliceId);
				if (!subTexture)
					continue;

				std::string entityName = layer.name + "_" +
					std::to_string(tile.position.gridX) + "_" +
					std::to_string(tile.position.gridY);

				Yuicy::Entity entity = scene->CreateEntity(entityName);

				glm::vec2 worldPos = GridToWorld(data, tile.position.gridX, tile.position.gridY);

				auto& transform = entity.GetComponent<Yuicy::TransformComponent>();
				transform.Translation = { worldPos.x, worldPos.y, 0.0f };
				transform.Rotation = { 0.0f, 0.0f, glm::radians(tile.transform.rotation) };
				transform.Scale = {
					static_cast<float>(tile.size.gridWidth),
					static_cast<float>(tile.size.gridHeight),
					1.0f
				};

				auto& sprite = entity.AddComponent<Yuicy::SpriteRendererComponent>();
				sprite.SubTexture = subTexture;
				sprite.FlipX = tile.transform.flipX;
				sprite.FlipY = tile.transform.flipY;
				sprite.Color.a = layer.opacity;
				sprite.SortingOrder = tile.zIndex;

				// 处理碰撞
				if (tile.collision.enabled)
				{
					auto& rb = entity.AddComponent<Yuicy::Rigidbody2DComponent>();
					rb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

					auto& collider = entity.AddComponent<Yuicy::BoxCollider2DComponent>();
					collider.Size = { 0.5f, 0.5f };
				}

				outEntities.push_back(entity);
			}
		}

		YUICY_CORE_INFO("DungeonMapBuilder: Built {} entities", outEntities.size());
	}

	Yuicy::Ref<Yuicy::SubTexture2D> DungeonMapBuilder::GetSubTextureBySliceId(TileMapData* data, const std::string& sliceId)
	{
		for (const auto& tileset : data->tilesets)
		{
			for (const auto& slice : tileset.slices)
			{
				if (slice.id == sliceId)
					return slice.subTexture;
			}
		}

		for (const auto& slice : data->slices)
		{
			if (slice.id == sliceId)
				return slice.subTexture;
		}

		return nullptr;
	}

	glm::vec2 DungeonMapBuilder::GridToWorld(TileMapData* data, int32_t gridX, int32_t gridY)
	{
		// Y轴翻转：编辑器坐标系 -> 游戏世界坐标系
		float worldX = static_cast<float>(gridX) + 0.5f;
		float worldY = static_cast<float>(data->map.height - 1 - gridY) + 0.5f;
		return { worldX, worldY };
	}
}
