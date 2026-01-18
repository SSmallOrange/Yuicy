#include <Yuicy.h>
#include "EnemyLoader.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <fstream>
#include <sstream>

namespace TinyDungeon {

	bool EnemyLoader::LoadConfig(const std::string& configPath)
	{
		std::ifstream file(configPath);
		if (!file.is_open())
		{
			YUICY_CORE_ERROR("EnemyLoader: Failed to open config file: {}", configPath);
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string jsonContent = buffer.str();
		file.close();

		if (!ParseJson(jsonContent))
		{
			YUICY_CORE_ERROR("EnemyLoader: Failed to parse enemy config");
			return false;
		}

		// Load sprite sheet
		if (!m_config.spriteSheet.empty())
		{
			m_spriteSheet = Yuicy::Texture2D::Create(m_config.spriteSheet);
			if (!m_spriteSheet)
			{
				YUICY_CORE_WARN("EnemyLoader: Failed to load sprite sheet: {}", m_config.spriteSheet);
			}
		}

		YUICY_CORE_INFO("EnemyLoader: Loaded {} enemy configs", m_config.enemies.size());
		return true;
	}

	bool EnemyLoader::ParseJson(const std::string& jsonContent)
	{
		rapidjson::Document doc;
		rapidjson::ParseResult result = doc.Parse(jsonContent.c_str());

		if (!result)
		{
			YUICY_CORE_ERROR("JSON parse error: {} (offset: {})",
				rapidjson::GetParseError_En(result.Code()), result.Offset());
			return false;
		}

		// Parse global config
		if (doc.HasMember("spriteSheet") && doc["spriteSheet"].IsString())
			m_config.spriteSheet = doc["spriteSheet"].GetString();

		if (doc.HasMember("cellSize") && doc["cellSize"].IsArray() && doc["cellSize"].Size() >= 2)
		{
			m_config.cellSize.x = doc["cellSize"][0].GetInt();
			m_config.cellSize.y = doc["cellSize"][1].GetInt();
		}

		// Parse enemies array
		if (doc.HasMember("enemies") && doc["enemies"].IsArray())
		{
			for (const auto& enemyJson : doc["enemies"].GetArray())
			{
				EnemyConfig enemy;

				if (enemyJson.HasMember("name")) enemy.name = enemyJson["name"].GetString();
				if (enemyJson.HasMember("tag")) enemy.tag = enemyJson["tag"].GetString();
				if (enemyJson.HasMember("script")) enemy.script = enemyJson["script"].GetString();

				// Parse spawn
				if (enemyJson.HasMember("spawn") && enemyJson["spawn"].IsObject())
				{
					const auto& spawn = enemyJson["spawn"];
					if (spawn.HasMember("position") && spawn["position"].IsArray() && spawn["position"].Size() >= 2)
					{
						enemy.position.x = spawn["position"][0].GetFloat();
						enemy.position.y = spawn["position"][1].GetFloat();
					}
					if (spawn.HasMember("zDepth")) enemy.position.z = spawn["zDepth"].GetFloat();
				}

				// Parse sprite
				if (enemyJson.HasMember("sprite") && enemyJson["sprite"].IsObject())
				{
					const auto& sprite = enemyJson["sprite"];
					if (sprite.HasMember("sortingOrder")) enemy.sortingOrder = sprite["sortingOrder"].GetInt();
					if (sprite.HasMember("flipX")) enemy.flipX = sprite["flipX"].GetBool();
				}

				// Parse animations
				if (enemyJson.HasMember("animations") && enemyJson["animations"].IsObject())
				{
					const auto& animations = enemyJson["animations"];
					if (animations.HasMember("default")) enemy.defaultAnimation = animations["default"].GetString();

					if (animations.HasMember("clips") && animations["clips"].IsArray())
					{
						for (const auto& clipJson : animations["clips"].GetArray())
						{
							EnemyAnimationClip clip;
							if (clipJson.HasMember("name")) clip.name = clipJson["name"].GetString();
							if (clipJson.HasMember("startCoord") && clipJson["startCoord"].IsArray() && clipJson["startCoord"].Size() >= 2)
							{
								clip.startCoord.x = clipJson["startCoord"][0].GetInt();
								clip.startCoord.y = clipJson["startCoord"][1].GetInt();
							}
							if (clipJson.HasMember("frameCount")) clip.frameCount = clipJson["frameCount"].GetInt();
							if (clipJson.HasMember("frameDuration")) clip.frameDuration = clipJson["frameDuration"].GetFloat();
							if (clipJson.HasMember("loop")) clip.loop = clipJson["loop"].GetBool();

							enemy.animationClips.push_back(std::move(clip));
						}
					}
				}

				// Parse physics
				if (enemyJson.HasMember("physics") && enemyJson["physics"].IsObject())
				{
					const auto& physics = enemyJson["physics"];
					if (physics.HasMember("bodyType"))
					{
						std::string bodyType = physics["bodyType"].GetString();
						if (bodyType == "static")
							enemy.bodyType = Yuicy::Rigidbody2DComponent::BodyType::Static;
						else if (bodyType == "kinematic")
							enemy.bodyType = Yuicy::Rigidbody2DComponent::BodyType::Kinematic;
						else
							enemy.bodyType = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
					}
					if (physics.HasMember("fixedRotation")) enemy.fixedRotation = physics["fixedRotation"].GetBool();
					if (physics.HasMember("gravityScale")) enemy.gravityScale = physics["gravityScale"].GetFloat();
				}

				// Parse collider
				if (enemyJson.HasMember("collider") && enemyJson["collider"].IsObject())
				{
					const auto& collider = enemyJson["collider"];
					if (collider.HasMember("type")) enemy.colliderType = collider["type"].GetString();
					if (collider.HasMember("radius")) enemy.colliderRadius = collider["radius"].GetFloat();
					if (collider.HasMember("friction")) enemy.friction = collider["friction"].GetFloat();

					if (collider.HasMember("categoryBits") && collider["categoryBits"].IsString())
						enemy.categoryBits = ParseCollisionLayer(collider["categoryBits"].GetString());

					if (collider.HasMember("maskBits") && collider["maskBits"].IsArray())
					{
						enemy.maskBits = 0;
						for (const auto& mask : collider["maskBits"].GetArray())
						{
							if (mask.IsString())
								enemy.maskBits |= ParseCollisionLayer(mask.GetString());
						}
					}
				}

				// Parse stats
				if (enemyJson.HasMember("stats") && enemyJson["stats"].IsObject())
				{
					const auto& stats = enemyJson["stats"];
					if (stats.HasMember("health")) enemy.health = stats["health"].GetFloat();
					if (stats.HasMember("damage")) enemy.damage = stats["damage"].GetFloat();
					if (stats.HasMember("speed")) enemy.speed = stats["speed"].GetFloat();
					if (stats.HasMember("detectRange")) enemy.detectRange = stats["detectRange"].GetFloat();
					if (stats.HasMember("attackRange")) enemy.attackRange = stats["attackRange"].GetFloat();
				}

				m_config.enemies.push_back(std::move(enemy));
			}
		}

		return true;
	}

	uint16_t EnemyLoader::ParseCollisionLayer(const std::string& layerName)
	{
		if (layerName == "Default") return Yuicy::CollisionLayer::Default;
		if (layerName == "Player") return Yuicy::CollisionLayer::Player;
		if (layerName == "Enemy") return Yuicy::CollisionLayer::Enemy;
		if (layerName == "Ground") return Yuicy::CollisionLayer::Ground;
		if (layerName == "Trigger") return Yuicy::CollisionLayer::Trigger;
		if (layerName == "Bullet") return Yuicy::CollisionLayer::Bullet;
		if (layerName == "All") return Yuicy::CollisionLayer::All;
		return Yuicy::CollisionLayer::Default;
	}

	std::vector<Yuicy::Entity> EnemyLoader::SpawnAllEnemies(Yuicy::Scene* scene)
	{
		std::vector<Yuicy::Entity> entities;

		for (const auto& config : m_config.enemies)
		{
			auto entity = SpawnEnemy(scene, config);
			if (entity)
				entities.push_back(entity);
		}

		YUICY_CORE_INFO("EnemyLoader: Spawned {} enemies", entities.size());
		return entities;
	}

	Yuicy::Entity EnemyLoader::SpawnEnemy(Yuicy::Scene* scene, const EnemyConfig& config)
	{
		if (!scene)
			return {};

		// Create entity
		Yuicy::Entity enemy = scene->CreateEntity(config.name);
		auto& tag = enemy.GetComponent<Yuicy::TagComponent>();
		tag.Tag = config.tag;

		// Transform
		auto& transform = enemy.GetComponent<Yuicy::TransformComponent>();
		transform.Translation = config.position;

		// Sprite
		auto& sprite = enemy.AddComponent<Yuicy::SpriteRendererComponent>();
		sprite.SortingOrder = config.sortingOrder;
		sprite.FlipX = config.flipX;

		// Animation
		if (!config.animationClips.empty() && m_spriteSheet)
		{
			auto& animComp = enemy.AddComponent<Yuicy::AnimationComponent>();

			for (const auto& clip : config.animationClips)
			{
				Yuicy::AnimationClip animClip(clip.name, clip.frameDuration, clip.loop);
				animClip.AddFramesFromSheet(
					m_spriteSheet,
					clip.startCoord,
					clip.frameCount,
					m_config.cellSize
				);
				animComp.AddClip(animClip);
			}

			if (!config.defaultAnimation.empty())
				animComp.Play(config.defaultAnimation);
		}

		// Physics
		auto& rb = enemy.AddComponent<Yuicy::Rigidbody2DComponent>();
		rb.Type = config.bodyType;
		rb.FixedRotation = config.fixedRotation;

		// Collider
		if (config.colliderType == "circle")
		{
			auto& collider = enemy.AddComponent<Yuicy::CircleCollider2DComponent>();
			collider.Radius = config.colliderRadius;
			collider.Friction = config.friction;
			collider.CategoryBits = config.categoryBits;
			collider.MaskBits = config.maskBits;
		}
		else
		{
			auto& collider = enemy.AddComponent<Yuicy::BoxCollider2DComponent>();
			collider.Size = { config.colliderRadius, config.colliderRadius };
			collider.Friction = config.friction;
			collider.CategoryBits = config.categoryBits;
			collider.MaskBits = config.maskBits;
		}

		// Lua Script (AI behavior)
		if (!config.script.empty())
		{
			enemy.AddComponent<Yuicy::LuaScriptComponent>(config.script);
		}

		YUICY_CORE_TRACE("EnemyLoader: Spawned enemy '{}' at ({}, {})", 
			config.name, config.position.x, config.position.y);

		return enemy;
	}

}
