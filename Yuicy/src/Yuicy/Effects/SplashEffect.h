#pragma once

#include "Yuicy/Core/Timestep.h"
#include <glm/glm.hpp>
#include <vector>

namespace Yuicy {

	// 溅射效果
	struct SplashConfig
	{
		glm::vec4 colorStart = { 0.6f, 0.7f, 0.9f, 0.8f };
		glm::vec4 colorEnd = { 0.5f, 0.6f, 0.8f, 0.0f };
		float sizeMin = 0.02f;
		float sizeMax = 0.05f;
		float speedMin = 1.0f;
		float speedMax = 2.5f;
		float lifetime = 0.3f;
		int particleCount = 6;
		float spreadAngle = 2.0f;		// 扩散角度
		glm::vec2 gravity = { 0.0f, -8.0f };
	};

	class SplashEffect
	{
	public:
		static void Emit(const glm::vec2& position, const SplashConfig& config);

		static void OnUpdate(Timestep ts);
		static void OnRender();

		static void Clear();

	private:
		struct Particle
		{
			glm::vec2 position;
			glm::vec2 velocity;
			glm::vec4 colorStart;
			glm::vec4 colorEnd;
			float size;
			float lifetime;
			float lifeRemaining;
			bool active = false;
		};

		static constexpr uint32_t MAX_PARTICLES = 500;
		static std::vector<Particle> s_ParticlePool;
		static uint32_t s_PoolIndex;
	};

}
