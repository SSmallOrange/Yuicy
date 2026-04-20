#pragma once

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

#include <string>
#include <vector>
#include <optional>

namespace Yuicy {

	// 实体快照：用于 Delete 命令的 Undo 恢复
	// 存储单个实体的所有组件数据，支持完整恢复
	struct EntitySnapshot
	{
		UUID uuid;
		std::string tag;
		TransformComponent transform;
		RelationshipComponent relationship;

		std::optional<SpriteRendererComponent> sprite;
		std::optional<CameraComponent> camera;
		std::optional<LuaScriptComponent> luaScript;
		std::optional<Rigidbody2DComponent> rigidbody;
		std::optional<BoxCollider2DComponent> boxCollider;
		std::optional<CircleCollider2DComponent> circleCollider;
		std::optional<AnimationComponent> animation;

		static EntitySnapshot Capture(Entity entity)
		{
			EntitySnapshot snapshot;
			snapshot.uuid = entity.GetUUID();
			snapshot.tag = entity.GetComponent<TagComponent>().Tag;
			snapshot.transform = entity.GetComponent<TransformComponent>();
			snapshot.relationship = entity.GetComponent<RelationshipComponent>();

			if (entity.HasComponent<SpriteRendererComponent>())
				snapshot.sprite = entity.GetComponent<SpriteRendererComponent>();
			if (entity.HasComponent<CameraComponent>())
				snapshot.camera = entity.GetComponent<CameraComponent>();
			if (entity.HasComponent<LuaScriptComponent>())
			{
				auto& lsc = entity.GetComponent<LuaScriptComponent>();
				LuaScriptComponent copy;
				copy.ScriptHandle = lsc.ScriptHandle;
				snapshot.luaScript = copy;
			}
			if (entity.HasComponent<Rigidbody2DComponent>())
			{
				auto rb = entity.GetComponent<Rigidbody2DComponent>();
				rb.RuntimeBody = nullptr;
				snapshot.rigidbody = rb;
			}
			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto bc = entity.GetComponent<BoxCollider2DComponent>();
				bc.RuntimeFixture = nullptr;
				snapshot.boxCollider = bc;
			}
			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto cc = entity.GetComponent<CircleCollider2DComponent>();
				cc.RuntimeFixture = nullptr;
				snapshot.circleCollider = cc;
			}
			if (entity.HasComponent<AnimationComponent>())
				snapshot.animation = entity.GetComponent<AnimationComponent>();

			return snapshot;
		}

		// 递归捕获实体及其所有后代
		static void CaptureHierarchy(Entity entity, Scene* scene, std::vector<EntitySnapshot>& outSnapshots)
		{
			outSnapshots.push_back(Capture(entity));

			for (auto childUUID : entity.Children())
			{
				Entity child = scene->FindEntityByUUID(childUUID);
				if (child)
					CaptureHierarchy(child, scene, outSnapshots);
			}
		}

		// 恢复快照中的实体到场景
		Entity Restore(Scene* scene) const
		{
			Entity entity = scene->CreateEntityWithUUID(uuid, tag);

			entity.GetComponent<TransformComponent>() = transform;

			// 恢复 Relationship（不含 Children，子实体会在各自恢复时重建）
			auto& rel = entity.GetComponent<RelationshipComponent>();
			rel.ParentHandle = relationship.ParentHandle;

			if (sprite)			entity.AddComponent<SpriteRendererComponent>(*sprite);
			if (camera)			entity.AddComponent<CameraComponent>(*camera);
			if (luaScript)		entity.AddComponent<LuaScriptComponent>(*luaScript);
			if (rigidbody)		entity.AddComponent<Rigidbody2DComponent>(*rigidbody);
			if (boxCollider)	entity.AddComponent<BoxCollider2DComponent>(*boxCollider);
			if (circleCollider)	entity.AddComponent<CircleCollider2DComponent>(*circleCollider);
			if (animation)		entity.AddComponent<AnimationComponent>(*animation);

			return entity;
		}
	};

}
