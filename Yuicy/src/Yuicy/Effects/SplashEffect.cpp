#include "pch.h"
#include "SplashEffect.h"
#include "Yuicy/Renderer/Renderer2D.h"

#include <cstdlib>
#include <cmath>

namespace Yuicy {

	std::vector<SplashEffect::Particle> SplashEffect::s_ParticlePool(MAX_PARTICLES);
	uint32_t SplashEffect::s_PoolIndex = 0;

	static float RandomFloat()
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	static float RandomRange(float min, float max)
	{
		return min + RandomFloat() * (max - min);
	}

	void SplashEffect::Emit(const glm::vec2& position, const SplashConfig& config)
	{
		for (int i = 0; i < config.particleCount; i++)
		{
			Particle& p = s_ParticlePool[s_PoolIndex];
			s_PoolIndex = (s_PoolIndex + 1) % MAX_PARTICLES;

			p.active = true;
			p.position = position;

			// 计算扇形向上的速度方向
			float baseAngle = 3.14159f * 0.5f;  // 向上 (PI/2)
			float angleOffset = RandomRange(-config.spreadAngle * 0.5f, config.spreadAngle * 0.5f);
			float angle = baseAngle + angleOffset;
			float speed = RandomRange(config.speedMin, config.speedMax);

			p.velocity.x = std::cos(angle) * speed;
			p.velocity.y = std::sin(angle) * speed;

			p.colorStart = config.colorStart;
			p.colorEnd = config.colorEnd;
			p.size = RandomRange(config.sizeMin, config.sizeMax);
			p.lifetime = config.lifetime;
			p.lifeRemaining = config.lifetime;
		}
	}

	void SplashEffect::OnUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);

		for (auto& p : s_ParticlePool)
		{
			if (!p.active)
				continue;

			p.lifeRemaining -= dt;
			if (p.lifeRemaining <= 0.0f)
			{
				p.active = false;
				continue;
			}

			// 重力
			p.velocity.y -= 8.0f * dt;

			// 位置更新
			p.position += p.velocity * dt;
		}
	}

	void SplashEffect::OnRender()
	{
		for (const auto& p : s_ParticlePool)
		{
			if (!p.active)
				continue;

			float t = p.lifeRemaining / p.lifetime;
			glm::vec4 color = glm::mix(p.colorEnd, p.colorStart, t);
			float size = p.size * t;

			Renderer2D::DrawQuad(
				{ p.position.x, p.position.y, 0.95f },
				{ size, size },
				color
			);
		}
	}

	void SplashEffect::Clear()
	{
		for (auto& p : s_ParticlePool)
		{
			p.active = false;
		}
	}

}
