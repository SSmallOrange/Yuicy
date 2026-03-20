#include <Yuicy.h>
#include "DungeonMapBuilder.h"
#include "Yuicy/TileMap/TileMapColliderMerger.h"

#include <ranges>
#include <algorithm>

namespace TinyDungeon {

	void DungeonMapBuilder::Build(Yuicy::ITileMapData* mapData, Yuicy::Scene* scene, std::vector<Yuicy::Entity>& outEntities)
	{
		TileMapData* data = dynamic_cast<TileMapData*>(mapData);
		if (!data)
		{
			YUICY_CORE_ERROR("DungeonMapBuilder: Invalid map data type");
			return;
		}

		// Create Entity
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

				glm::vec3 worldPos = GridToWorld(data,
					static_cast<float>(tile.position.gridX) + 0.5f,
					static_cast<float>(tile.position.gridY) + 0.5f,
					tile.zIndex);

				auto& transform = entity.GetComponent<Yuicy::TransformComponent>();
				transform.Translation = { worldPos.x, worldPos.y, worldPos.z };
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

				// 未启用碰撞体合并时，逐个创建瓦片
				if (!IsColliderMergingEnabled() && tile.collision.enabled)
				{
					BuildIndividualCollider(entity, tile);
				}

				outEntities.push_back(entity);
			}
		}

		// 碰撞体合并
		if (IsColliderMergingEnabled())
		{
			auto collisionGrid = BuildCollisionGrid(data);
			auto mergedColliders = Yuicy::TileMapColliderMerger::Merge(collisionGrid);
			BuildMergedColliderEntities(data, scene, mergedColliders, outEntities);
		}

		YUICY_CORE_INFO("DungeonMapBuilder: Built {} entities", outEntities.size());
	}

	std::vector<std::vector<bool>> DungeonMapBuilder::BuildCollisionGrid(TileMapData* data) const
	{
		std::vector<std::vector<bool>> grid(data->map.height, std::vector<bool>(data->map.width, false));

		for (const auto& layer : data->layers)
		{
			if (!layer.visible)
				continue;

			for (const auto& tile : layer.tiles)
			{
				if (!tile.collision.enabled)
					continue;

				// 标记 tile 覆盖的所有格子（支持多格 tile）
				for (int32_t dy = 0; dy < tile.size.gridHeight; ++dy)
				{
					for (int32_t dx = 0; dx < tile.size.gridWidth; ++dx)
					{
						int32_t gx = tile.position.gridX + dx;
						int32_t gy = tile.position.gridY + dy;
						if (gy >= 0 && gy < data->map.height && gx >= 0 && gx < data->map.width)
							grid[gy][gx] = true;
					}
				}
			}
		}

		return grid;
	}

	void DungeonMapBuilder::BuildMergedColliderEntities(TileMapData* data, Yuicy::Scene* scene,
		const std::vector<Yuicy::MergedColliderRect>& mergedColliders, std::vector<Yuicy::Entity>& outEntities)
	{
		for (const auto& [x, y, width, height] : mergedColliders)
		{
			std::string entityName = "Ground_Collider_" +
				std::to_string(x) + "_" + std::to_string(y) +
				"_" + std::to_string(width) + "x" + std::to_string(height);

			Yuicy::Entity entity = scene->CreateEntity(entityName);

			// 计算合并矩形中心的世界坐标
			float centerGridX = static_cast<float>(x) + width / 2.0f;
			float centerGridY = static_cast<float>(y) + height / 2.0f;
			glm::vec3 worldPos = GridToWorld(data, centerGridX, centerGridY, 0);

			auto& transform = entity.GetComponent<Yuicy::TransformComponent>();
			transform.Translation = worldPos;
			transform.Scale = {
				static_cast<float>(width),
				static_cast<float>(height),
				1.0f
			};

			auto& rb = entity.AddComponent<Yuicy::Rigidbody2DComponent>();
			rb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

			auto& collider = entity.AddComponent<Yuicy::BoxCollider2DComponent>();
			collider.Size = { 0.5f, 0.5f };	// 半尺寸，配合 Scale 形成完整碰撞区域
			collider.CategoryBits = Yuicy::CollisionLayer::Ground;

			outEntities.push_back(entity);
		}
	}

	void DungeonMapBuilder::BuildIndividualCollider(
		Yuicy::Entity entity, const TileInstance& tile)
	{
		auto& rb = entity.AddComponent<Yuicy::Rigidbody2DComponent>();
		rb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

		auto& collider = entity.AddComponent<Yuicy::BoxCollider2DComponent>();
		collider.Size = { 0.5f, 0.5f };
		collider.CategoryBits = Yuicy::CollisionLayer::Ground;
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

	glm::vec3 DungeonMapBuilder::GridToWorld(TileMapData* data, float gridX, float gridY, int32_t gridZ)
	{
		// Y轴翻转：编辑器坐标系 -> 游戏世界坐标系
		float worldX = gridX;
		float worldY = static_cast<float>(data->map.height) - gridY;
		float worldZ = static_cast<float>(gridZ) * 0.05f;  // 以层数做Z轴，10 -> 0.1 11 -> 0.11
		return { worldX, worldY, worldZ };
	}
}
