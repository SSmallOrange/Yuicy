#include "pch.h"
#include "TilemapColliderBuilder.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Tilemap/TilemapColliderGeometry.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <array>
#include <cmath>

#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_world.h>

namespace Yuicy {

	namespace {

		const GridComponent* ResolveGrid(Entity tilemapEntity)
		{
			if (tilemapEntity.HasComponent<GridComponent>())
				return &tilemapEntity.GetComponent<GridComponent>();

			Entity parent = tilemapEntity.GetParent();
			if (parent && parent.HasComponent<GridComponent>())
				return &parent.GetComponent<GridComponent>();

			return nullptr;
		}

		bool IsValidPolygon(const std::array<b2Vec2, 4>& vertices)
		{
			float area = 0.0f;
			for (size_t i = 0; i < vertices.size(); i++)
			{
				const b2Vec2& a = vertices[i];
				const b2Vec2& b = vertices[(i + 1) % vertices.size()];
				area += a.x * b.y - b.x * a.y;
			}

			return std::abs(area) > 0.000001f;
		}

		std::array<b2Vec2, 4> ToBodyLocalVertices(const TilemapColliderShape& shape, const b2Vec2& bodyPosition)
		{
			std::array<b2Vec2, 4> vertices;
			for (size_t i = 0; i < shape.points.size(); i++)
				vertices[i].Set(shape.points[i].x - bodyPosition.x, shape.points[i].y - bodyPosition.y);

			return vertices;
		}

		std::vector<TilemapColliderShape> BuildColliderShapes(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider)
		{
			switch (collider.compositeOperation)
			{
				case TilemapColliderCompositeOperation::None:
					return TilemapColliderGeometry::BuildGridShapes(tilemapWorldTransform, grid, tilemap, collider);
				case TilemapColliderCompositeOperation::Merge:
					return TilemapColliderGeometry::BuildMergedGridShapes(tilemapWorldTransform, grid, tilemap, collider);
			}

			return TilemapColliderGeometry::BuildMergedGridShapes(tilemapWorldTransform, grid, tilemap, collider);
		}

	}

	void TilemapColliderBuilder::Build(Scene* scene, b2World* world, Entity tilemapEntity)
	{
		if (!scene || !world || !tilemapEntity)
			return;

		if (tilemapEntity.GetScene() != scene)
		{
			YUICY_CORE_WARN("TilemapColliderBuilder skipped entity from a different scene.");
			return;
		}

		if (!tilemapEntity.HasComponent<TilemapComponent>() || !tilemapEntity.HasComponent<TilemapCollider2DComponent>())
			return;

		auto& tilemap = tilemapEntity.GetComponent<TilemapComponent>();
		auto& collider = tilemapEntity.GetComponent<TilemapCollider2DComponent>();
		if (collider.runtimeBody || !collider.runtimeFixtures.empty())
		{
			YUICY_CORE_WARN("TilemapColliderBuilder skipped entity '{}' because it already has runtime collider data.", tilemapEntity.GetComponent<TagComponent>().Tag);
			return;
		}

		const GridComponent* grid = ResolveGrid(tilemapEntity);
		if (!grid)
		{
			YUICY_CORE_WARN("TilemapColliderBuilder skipped entity '{}' because no GridComponent was found.", tilemapEntity.GetComponent<TagComponent>().Tag);
			return;
		}

		if (grid->m_layout != GridLayout::Rectangular)
		{
			YUICY_CORE_WARN("TilemapColliderBuilder skipped entity '{}' because only Rectangular Grid collider is supported.", tilemapEntity.GetComponent<TagComponent>().Tag);
			return;
		}

		const glm::mat4 tilemapWorldTransform = scene->GetWorldSpaceTransformMatrix(tilemapEntity);
		std::vector<TilemapColliderShape> shapes = BuildColliderShapes(tilemapWorldTransform, *grid, tilemap, collider);
		if (shapes.empty())
			return;

		const glm::vec4 worldOrigin = tilemapWorldTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		const b2Vec2 bodyPosition(worldOrigin.x, worldOrigin.y);

		b2BodyDef bodyDef;
		bodyDef.type = b2_staticBody;
		bodyDef.position = bodyPosition;
		bodyDef.angle = 0.0f;

		b2Body* body = world->CreateBody(&bodyDef);
		body->GetUserData().pointer = (uintptr_t)tilemapEntity.GetEntityId();

		collider.runtimeBody = body;
		collider.runtimeFixtures.reserve(shapes.size());

		for (const TilemapColliderShape& shape : shapes)
		{
			std::array<b2Vec2, 4> vertices = ToBodyLocalVertices(shape, bodyPosition);
			if (!IsValidPolygon(vertices))
				continue;

			b2PolygonShape polygonShape;
			polygonShape.Set(vertices.data(), (int32)vertices.size());

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &polygonShape;
			fixtureDef.density = collider.density;
			fixtureDef.friction = collider.friction;
			fixtureDef.restitution = collider.restitution;
			fixtureDef.restitutionThreshold = collider.restitutionThreshold;
			fixtureDef.filter.categoryBits = collider.categoryBits;
			fixtureDef.filter.maskBits = collider.maskBits;
			fixtureDef.isSensor = collider.isTrigger;

			b2Fixture* fixture = body->CreateFixture(&fixtureDef);
			collider.runtimeFixtures.push_back(fixture);
		}

		if (collider.runtimeFixtures.empty())
		{
			world->DestroyBody(body);
			ClearRuntimeData(collider);
		}
	}

	void TilemapColliderBuilder::Rebuild(Scene* scene, b2World* world, Entity tilemapEntity)
	{
		if (!scene || !world || !tilemapEntity)
			return;

		if (tilemapEntity.GetScene() != scene)
		{
			YUICY_CORE_WARN("TilemapColliderBuilder skipped rebuild for entity from a different scene.");
			return;
		}

		if (!tilemapEntity.HasComponent<TilemapCollider2DComponent>())
			return;

		auto& collider = tilemapEntity.GetComponent<TilemapCollider2DComponent>();
		if (collider.runtimeBody)
			world->DestroyBody(static_cast<b2Body*>(collider.runtimeBody));

		ClearRuntimeData(collider);
		Build(scene, world, tilemapEntity);
	}

	void TilemapColliderBuilder::ClearRuntimeData(TilemapCollider2DComponent& collider)
	{
		collider.runtimeBody = nullptr;
		collider.runtimeFixtures.clear();
	}

}
