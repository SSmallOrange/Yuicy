#pragma once

#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Renderer/Camera.h"
#include "Yuicy/Effects/WeatherTypes.h"

#include <vector>
#include <glm/glm.hpp>

namespace Yuicy {

	class WeatherSystem
	{
	public:
		// maxParticles: 粒子池大小
		WeatherSystem(uint32_t maxParticles = 5000);
		~WeatherSystem() = default;

		void setWeather(WeatherType type, float intensity = 1.0f);
		void setWeather(const WeatherConfig& config);

		void clear();
		void setWindStrength(float strength);

		// Update particle states
		void onUpdate(Timestep ts);
		// Render particles
		void onRender(const glm::vec2& cameraPos, const glm::vec2& viewportSize);

	public:
		WeatherType getCurrentWeather() const { return m_config.type; }
		float getIntensity() const { return m_config.intensity; }
		bool isActive() const { return m_config.type != WeatherType::None; }

		static WeatherConfig getPreset(WeatherType type);

	private:
		void emitParticles(float deltaTime, const glm::vec2& cameraPos, const glm::vec2& viewportSize);
		float randomFloat();
		float randomRange(float min, float max);

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
		};

		WeatherConfig m_config;
		std::vector<Particle> m_particlePool;       // 粒子对象池
		uint32_t m_poolIndex = 0;
		float m_spawnAccumulator = 0.0f;            // 生成计时器累加器

		// camera info for spawning
		glm::vec2 m_lastCameraPos = { 0.0f, 0.0f };
		glm::vec2 m_lastViewportSize = { 10.0f, 10.0f };
	};

}