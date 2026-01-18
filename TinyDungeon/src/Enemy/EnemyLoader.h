#pragma once

#include <Yuicy.h>
#include <string>
#include <vector>

namespace TinyDungeon {

	struct EnemyAnimationClip
	{
		std::string name;
		glm::ivec2 startCoord = { 0, 0 };
		int frameCount = 1;
		float frameDuration = 0.5f;
		bool loop = true;
	};

	struct EnemyConfig
	{
		std::string name;
		std::string tag = "Enemy";
		std::string script;

		// Spawn
		glm::vec3 position = { 0.0f, 0.0f, 0.85f };
		int sortingOrder = 900;
		bool flipX = false;

		// Animations
		std::string defaultAnimation = "idle";
		std::vector<EnemyAnimationClip> animationClips;

		// Physics
		Yuicy::Rigidbody2DComponent::BodyType bodyType = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
		bool fixedRotation = true;
		float gravityScale = 1.0f;

		// Collider
		std::string colliderType = "circle";
		float colliderRadius = 0.4f;
		float friction = 0.0f;
		uint16_t categoryBits = Yuicy::CollisionLayer::Enemy;
		uint16_t maskBits = Yuicy::CollisionLayer::Default | Yuicy::CollisionLayer::Player | 
			Yuicy::CollisionLayer::Ground | Yuicy::CollisionLayer::Bullet;

		// Stats (passed to Lua script)
		float health = 5.0f;
		float damage = 1.0f;
		float speed = 2.0f;
		float detectRange = 5.0f;
		float attackRange = 1.0f;
	};

	struct EnemiesConfig
	{
		std::string spriteSheet;
		glm::ivec2 cellSize = { 24, 24 };
		std::vector<EnemyConfig> enemies;
	};


	class EnemyLoader
	{
	public:
		EnemyLoader() = default;
		~EnemyLoader() = default;

		bool LoadConfig(const std::string& configPath);

		std::vector<Yuicy::Entity> SpawnAllEnemies(Yuicy::Scene* scene);
		Yuicy::Entity SpawnEnemy(Yuicy::Scene* scene, const EnemyConfig& config);

		const EnemiesConfig& GetConfig() const { return m_config; }

	private:
		bool ParseJson(const std::string& jsonContent);
		uint16_t ParseCollisionLayer(const std::string& layerName);

	private:
		EnemiesConfig m_config;
		Yuicy::Ref<Yuicy::Texture2D> m_spriteSheet;
	};

}
