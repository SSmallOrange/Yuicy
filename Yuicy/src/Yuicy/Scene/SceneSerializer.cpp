#include "pch.h"
#include "Yuicy/Scene/SceneSerializer.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Utilities/YAMLSerializationHelpers.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <filesystem>

namespace Yuicy {

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_scene(scene)
	{
	}

	void SceneSerializer::SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		UUID uuid = entity.GetComponent<IDComponent>().ID;
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity";
		out << YAML::Value << (uint64_t)uuid;

		// TagComponent
		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap;

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap;
		}

		// RelationshipComponent
		if (entity.HasComponent<RelationshipComponent>())
		{
			auto& relationshipComponent = entity.GetComponent<RelationshipComponent>();
			out << YAML::Key << "Parent" << YAML::Value << (uint64_t)relationshipComponent.ParentHandle;

			out << YAML::Key << "Children";
			out << YAML::Value << YAML::BeginSeq;

			for (auto child : relationshipComponent.Children)
			{
				out << YAML::BeginMap;
				out << YAML::Key << "Handle" << YAML::Value << (uint64_t)child;
				out << YAML::EndMap;
			}
			out << YAML::EndSeq;
		}

		// TransformComponent
		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap;

			auto& transform = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Position" << YAML::Value << transform.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << transform.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << transform.Scale;

			out << YAML::EndMap;
		}

		// SpriteRendererComponent
		if (entity.HasComponent<SpriteRendererComponent>())
		{
			out << YAML::Key << "SpriteRendererComponent";
			out << YAML::BeginMap;

			auto& sprite = entity.GetComponent<SpriteRendererComponent>();
			out << YAML::Key << "Color" << YAML::Value << sprite.Color;
			out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)sprite.TextureHandle;
			out << YAML::Key << "TilingFactor" << YAML::Value << sprite.TilingFactor;
			out << YAML::Key << "FlipX" << YAML::Value << sprite.FlipX;
			out << YAML::Key << "FlipY" << YAML::Value << sprite.FlipY;
			out << YAML::Key << "SortingOrder" << YAML::Value << sprite.SortingOrder;

			out << YAML::EndMap;
		}

		// CameraComponent
		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap;

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap;
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap;

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap;
		}

		// LuaScriptComponent
		if (entity.HasComponent<LuaScriptComponent>())
		{
			out << YAML::Key << "LuaScriptComponent";
			out << YAML::BeginMap;

			auto& luaScript = entity.GetComponent<LuaScriptComponent>();
			out << YAML::Key << "ScriptHandle" << YAML::Value << (uint64_t)luaScript.ScriptHandle;

			out << YAML::EndMap;
		}

		// Rigidbody2DComponent
		if (entity.HasComponent<Rigidbody2DComponent>())
		{
			out << YAML::Key << "Rigidbody2DComponent";
			out << YAML::BeginMap;

			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();
			out << YAML::Key << "BodyType" << YAML::Value << (int)rb2d.Type;
			out << YAML::Key << "FixedRotation" << YAML::Value << rb2d.FixedRotation;

			out << YAML::EndMap;
		}

		// BoxCollider2DComponent
		if (entity.HasComponent<BoxCollider2DComponent>())
		{
			out << YAML::Key << "BoxCollider2DComponent";
			out << YAML::BeginMap;

			auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << bc2d.Offset;
			out << YAML::Key << "Size" << YAML::Value << bc2d.Size;
			out << YAML::Key << "Density" << YAML::Value << bc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << bc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << bc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << bc2d.RestitutionThreshold;
			out << YAML::Key << "CategoryBits" << YAML::Value << bc2d.CategoryBits;
			out << YAML::Key << "MaskBits" << YAML::Value << bc2d.MaskBits;
			out << YAML::Key << "IsTrigger" << YAML::Value << bc2d.IsTrigger;

			out << YAML::EndMap;
		}

		// CircleCollider2DComponent
		if (entity.HasComponent<CircleCollider2DComponent>())
		{
			out << YAML::Key << "CircleCollider2DComponent";
			out << YAML::BeginMap;

			auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();
			out << YAML::Key << "Offset" << YAML::Value << cc2d.Offset;
			out << YAML::Key << "Radius" << YAML::Value << cc2d.Radius;
			out << YAML::Key << "Density" << YAML::Value << cc2d.Density;
			out << YAML::Key << "Friction" << YAML::Value << cc2d.Friction;
			out << YAML::Key << "Restitution" << YAML::Value << cc2d.Restitution;
			out << YAML::Key << "RestitutionThreshold" << YAML::Value << cc2d.RestitutionThreshold;
			out << YAML::Key << "CategoryBits" << YAML::Value << cc2d.CategoryBits;
			out << YAML::Key << "MaskBits" << YAML::Value << cc2d.MaskBits;
			out << YAML::Key << "IsTrigger" << YAML::Value << cc2d.IsTrigger;

			out << YAML::EndMap;
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Scene";
		out << YAML::Value << m_scene->GetName();

		out << YAML::Key << "Entities";
		out << YAML::Value << YAML::BeginSeq;

		// 按 UUID 排序实体
		std::map<UUID, entt::entity> sortedEntityMap;
		auto idComponentView = m_scene->m_Registry.view<IDComponent>();
		for (auto entity : idComponentView)
			sortedEntityMap[idComponentView.get<IDComponent>(entity).ID] = entity;

		for (auto [id, entity] : sortedEntityMap)
			SerializeEntity(out, { entity, m_scene.get() });

		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();

		YUICY_CORE_INFO("Scene saved to: {}", filepath.string());
	}

	void SceneSerializer::DeserializeEntities(YAML::Node& entitiesNode)
	{
		for (auto entity : entitiesNode)
		{
			uint64_t uuid = entity["Entity"].as<uint64_t>();

			std::string name;
			if (auto tagComponent = entity["TagComponent"]; tagComponent)
				name = tagComponent["Tag"].as<std::string>();

			Entity deserializedEntity = m_scene->CreateEntityWithUUID(uuid, name);

			// RelationshipComponent
			auto& relationshipComponent = deserializedEntity.GetComponent<RelationshipComponent>();
			uint64_t parentHandle = entity["Parent"] ? entity["Parent"].as<uint64_t>() : 0;
			relationshipComponent.ParentHandle = parentHandle;

			if (auto children = entity["Children"]; children)
			{
				for (auto child : children)
				{
					uint64_t childHandle = child["Handle"].as<uint64_t>();
					relationshipComponent.Children.push_back(childHandle);
				}
			}

			// TransformComponent
			if (auto transformComponent = entity["TransformComponent"]; transformComponent)
			{
				auto& transform = deserializedEntity.GetComponent<TransformComponent>();
				transform.Translation = transformComponent["Position"].as<glm::vec3>(glm::vec3(0.0f));
				transform.Rotation = transformComponent["Rotation"].as<glm::vec3>(glm::vec3(0.0f));
				transform.Scale = transformComponent["Scale"].as<glm::vec3>(glm::vec3(1.0f));
			}

			// SpriteRendererComponent
			if (auto spriteRendererComponent = entity["SpriteRendererComponent"]; spriteRendererComponent)
			{
				auto& sprite = deserializedEntity.AddComponent<SpriteRendererComponent>();
				sprite.Color = spriteRendererComponent["Color"].as<glm::vec4>(glm::vec4(1.0f));

				// TextureHandle
				if (spriteRendererComponent["TextureHandle"])
					sprite.TextureHandle = spriteRendererComponent["TextureHandle"].as<uint64_t>(0);

				sprite.TilingFactor = spriteRendererComponent["TilingFactor"].as<float>(1.0f);
				sprite.FlipX = spriteRendererComponent["FlipX"].as<bool>(false);
				sprite.FlipY = spriteRendererComponent["FlipY"].as<bool>(false);
				sprite.SortingOrder = spriteRendererComponent["SortingOrder"].as<int>(0);
			}

			// CameraComponent
			if (auto cameraComponent = entity["CameraComponent"]; cameraComponent)
			{
				auto& cc = deserializedEntity.AddComponent<CameraComponent>();

				auto cameraNode = cameraComponent["Camera"];
				if (cameraNode)
				{
					cc.Camera.SetOrthographicSize(cameraNode["OrthographicSize"].as<float>(10.0f));
					cc.Camera.SetOrthographicNearClip(cameraNode["OrthographicNear"].as<float>(-1.0f));
					cc.Camera.SetOrthographicFarClip(cameraNode["OrthographicFar"].as<float>(1.0f));
				}

				cc.Primary = cameraComponent["Primary"].as<bool>(true);
				cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>(false);
			}

			// LuaScriptComponent
			if (auto luaScriptComponent = entity["LuaScriptComponent"]; luaScriptComponent)
			{
				auto& luaScript = deserializedEntity.AddComponent<LuaScriptComponent>();

				if (luaScriptComponent["ScriptHandle"])
					luaScript.ScriptHandle = luaScriptComponent["ScriptHandle"].as<uint64_t>(0);
			}

			// Rigidbody2DComponent
			if (auto rigidbody2DComponent = entity["Rigidbody2DComponent"]; rigidbody2DComponent)
			{
				auto& rb2d = deserializedEntity.AddComponent<Rigidbody2DComponent>();
				rb2d.Type = (Rigidbody2DComponent::BodyType)rigidbody2DComponent["BodyType"].as<int>(0);
				rb2d.FixedRotation = rigidbody2DComponent["FixedRotation"].as<bool>(false);
			}

			// BoxCollider2DComponent
			if (auto boxCollider2DComponent = entity["BoxCollider2DComponent"]; boxCollider2DComponent)
			{
				auto& bc2d = deserializedEntity.AddComponent<BoxCollider2DComponent>();
				bc2d.Offset = boxCollider2DComponent["Offset"].as<glm::vec2>(glm::vec2(0.0f));
				bc2d.Size = boxCollider2DComponent["Size"].as<glm::vec2>(glm::vec2(0.5f));
				bc2d.Density = boxCollider2DComponent["Density"].as<float>(1.0f);
				bc2d.Friction = boxCollider2DComponent["Friction"].as<float>(0.5f);
				bc2d.Restitution = boxCollider2DComponent["Restitution"].as<float>(0.0f);
				bc2d.RestitutionThreshold = boxCollider2DComponent["RestitutionThreshold"].as<float>(0.5f);
				bc2d.CategoryBits = boxCollider2DComponent["CategoryBits"].as<uint16_t>(CollisionLayer::Default);
				bc2d.MaskBits = boxCollider2DComponent["MaskBits"].as<uint16_t>(CollisionLayer::All);
				bc2d.IsTrigger = boxCollider2DComponent["IsTrigger"].as<bool>(false);
			}

			// CircleCollider2DComponent
			if (auto circleCollider2DComponent = entity["CircleCollider2DComponent"]; circleCollider2DComponent)
			{
				auto& cc2d = deserializedEntity.AddComponent<CircleCollider2DComponent>();
				cc2d.Offset = circleCollider2DComponent["Offset"].as<glm::vec2>(glm::vec2(0.0f));
				cc2d.Radius = circleCollider2DComponent["Radius"].as<float>(0.5f);
				cc2d.Density = circleCollider2DComponent["Density"].as<float>(1.0f);
				cc2d.Friction = circleCollider2DComponent["Friction"].as<float>(0.5f);
				cc2d.Restitution = circleCollider2DComponent["Restitution"].as<float>(0.0f);
				cc2d.RestitutionThreshold = circleCollider2DComponent["RestitutionThreshold"].as<float>(0.5f);
				cc2d.CategoryBits = circleCollider2DComponent["CategoryBits"].as<uint16_t>(CollisionLayer::Default);
				cc2d.MaskBits = circleCollider2DComponent["MaskBits"].as<uint16_t>(CollisionLayer::All);
				cc2d.IsTrigger = circleCollider2DComponent["IsTrigger"].as<bool>(false);
			}
		}
	}

	bool SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			YUICY_CORE_ERROR("Failed to open scene file: {}", filepath.string());
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		YUICY_CORE_INFO("Deserializing scene '{}'", sceneName);
		m_scene->SetName(sceneName);

		auto entities = data["Entities"];
		if (entities)
			DeserializeEntities(entities);

		return true;
	}

}
