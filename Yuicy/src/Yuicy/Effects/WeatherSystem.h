#pragma once

#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Renderer/Camera.h"
#include "Yuicy/Effects/WeatherTypes.h"
#include "Yuicy/Physics/Physics2D.h"

#include <vector>
#include <glm/glm.hpp>

namespace Yuicy {

	class WeatherSystem
	{
	public:
		// maxParticles: 粒子池大小
		WeatherSystem(uint32_t maxParticles = 5000);
		~WeatherSystem() = default;

		void SetWeather(WeatherType type, WeatherIntensity intensity = WeatherIntensity::Normal);
		void SetWeather(const WeatherConfig& config);

		// 平滑过渡
		void TransitionTo(WeatherType type, WeatherIntensity intensity = WeatherIntensity::Normal);
		void TransitionTo(const WeatherConfig& config);
		void TransitionTo(const std::string& presetName);

		void Clear();
		void FadeOut(float duration = 2.0f);  // 淡出

		void SetWindStrength(float strength);
		void SetIntensity(float intensity);

		WeatherConfig& getConfig() { return m_currentConfig; }

		// Update particle states
		void OnUpdate(Timestep ts);
		// Render particles
		void OnRender(const glm::vec2& cameraPos, const glm::vec2& viewportSize);

		// 设置物理系统
		void SetPhysics2D(Physics2D* physics) { m_physics2D = physics; }

	public:
		WeatherType GetCurrentWeather() const { return m_currentConfig.type; }
		const std::string& GetCurrentWeatherName() const { return m_currentConfig.name; }
		float GetIntensity() const { return m_currentConfig.intensity; }
		float GetWindStrength() const { return m_currentConfig.windStrength; }
		bool IsActive() const { return m_currentConfig.type != WeatherType::None; }
		bool IsTransitioning() const { return m_isTransitioning; }
		float GetTransitionProgress() const { return m_transitionProgress; }

	private:
		void EmitParticles(float deltaTime, const glm::vec2& cameraPos, const glm::vec2& viewportSize);
		float RandomFloat();
		float RandomRange(float min, float max);

		void UpdateTransition(Timestep ts);

		// 物理雨滴相关
		void EmitPhysicsRaindrop(const glm::vec2& cameraPos, const glm::vec2& viewportSize);
		void UpdatePhysicsRaindrops(float dt);

	private:
		struct Particle
		{
			glm::vec2 position = { 0.0f, 0.0f };
			glm::vec2 velocity = { 0.0f, 0.0f };
			glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
			float size = 0.05f;
			float rotation = 0.0f;
			float life = 0.0f;          // 剩余生命
			float maxLife = 1.0f;       // 初始生命
			bool active = false;

			float phaseOffset = 0.0f;   // 用于运动相位偏移
		};

		void ApplyParticleMotion(Particle& particle, float dt);

		// 物理雨滴结构
		struct PhysicsRaindrop
		{
			glm::vec2 position = { 0.0f, 0.0f };
			glm::vec2 velocity = { 0.0f, -8.0f };
			bool active = false;
		};

	private:

		// 当前配置
		WeatherConfig m_currentConfig;

		// 过渡状态
		bool m_isTransitioning = false;
		float m_transitionProgress = 0.0f;		// 过渡进度
		float m_transitionDuration = 2.0f;		// 过渡间隔
		WeatherConfig m_targetConfig;			// 目标状态
		WeatherConfig m_previousConfig;			// 先前状态

		std::vector<Particle> m_particlePool;       // 粒子对象池
		uint32_t m_poolIndex = 0;
		float m_spawnAccumulator = 0.0f;            // 生成计时器累加器

		// camera info for spawning
		glm::vec2 m_lastCameraPos = { 0.0f, 0.0f };
		glm::vec2 m_lastViewportSize = { 10.0f, 10.0f };

		float m_globalTime = 0.0f;

		// 物理雨滴
		static constexpr uint32_t MAX_PHYSICS_RAINDROPS = 200;
		std::vector<PhysicsRaindrop> m_physicsRaindrops;
		uint32_t m_physicsRaindropIndex = 0;
		float m_physicsSpawnAccumulator = 0.0f;
		Physics2D* m_physics2D = nullptr;
	};

}