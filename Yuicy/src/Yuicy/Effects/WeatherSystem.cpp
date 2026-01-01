#include "pch.h"
#include "WeatherSystem.h"
#include "Yuicy/Renderer/Renderer2D.h"

#include <cstdlib>
#include <ctime>

namespace Yuicy {

	WeatherSystem::WeatherSystem(uint32_t maxParticles)
	{
		m_particlePool.resize(maxParticles);
		m_poolIndex = 0;

		std::srand(static_cast<unsigned>(std::time(nullptr)));
	}

	void WeatherSystem::setWeather(WeatherType type, float intensity)
	{
		m_config = getPreset(type);
		m_config.intensity = intensity;
	}

	void WeatherSystem::setWeather(const WeatherConfig& config)
	{
		m_config = config;
	}

	void WeatherSystem::clear()
	{
		m_config.type = WeatherType::None;  // 粒子自然消亡
	}

	void WeatherSystem::setWindStrength(float strength)
	{
		m_config.windStrength = glm::clamp(strength, -1.0f, 1.0f);
	}

	void WeatherSystem::onUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);

		for (auto& particle : m_particlePool)
		{
			if (!particle.active)
				continue;

			particle.life -= dt;
			if (particle.life <= 0.0f)
			{
				particle.active = false;
				continue;
			}

			// 重力
			particle.velocity += m_config.particles.gravity * dt;

			// 风力影响：windStrength * 基础风速系数
			float windEffect = m_config.windStrength * 2.0f;
			particle.position.x += windEffect * dt;

			// 位置
			particle.position += particle.velocity * dt;

			if (m_config.type == WeatherType::Snow)
			{
				// 使用正弦函数产生飘动效果
				float sway = std::sin(particle.life * 5.0f) * 0.3f * dt;
				particle.position.x += sway;
				particle.rotation += dt * 0.5f;
			}
		}
	}

	void WeatherSystem::onRender(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		m_lastCameraPos = cameraPos;
		m_lastViewportSize = viewportSize;

		if (m_config.type != WeatherType::None)
		{
			emitParticles(1.0f / 60.0f, cameraPos, viewportSize);
		}

		for (const auto& particle : m_particlePool)
		{
			if (!particle.active)
				continue;

			// 计算生命周期进度 [0, 1]
			float lifeProgress = particle.life / particle.maxLife;

			// 颜色插值
			glm::vec4 color = glm::mix(m_config.particles.colorEnd, m_config.particles.colorStart, lifeProgress);

			// 透明度衰减
			color.a *= lifeProgress;

			if (m_config.particles.texture)
			{
				Renderer2D::DrawRotatedQuad(
					{ particle.position.x, particle.position.y, 0.9f },
					{ particle.size, particle.size },
					particle.rotation,
					m_config.particles.texture,
					1.0f,
					color
				);
			}
			else
			{
				Renderer2D::DrawRotatedQuad(
					{ particle.position.x, particle.position.y, 0.9f },
					{ particle.size, particle.size },
					particle.rotation,
					color
				);
			}
		}
	}

	void WeatherSystem::emitParticles(float deltaTime, const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		// 计算本帧应该生成的粒子数量
		// spawnRate * intensity = 实际生成速率
		float spawnRate = m_config.particles.spawnRate * m_config.intensity;
		m_spawnAccumulator += spawnRate * deltaTime;

		// 生成区域
		float spawnWidth = viewportSize.x * m_config.particles.spawnWidthMultiplier;
		float spawnY = cameraPos.y + viewportSize.y * m_config.particles.spawnHeightOffset;
		float spawnXMin = cameraPos.x - spawnWidth * 0.5f;
		float spawnXMax = cameraPos.x + spawnWidth * 0.5f;

		// 生成累积的粒子
		while (m_spawnAccumulator >= 1.0f)
		{
			m_spawnAccumulator -= 1.0f;
			Particle& particle = m_particlePool[m_poolIndex];

			// 激活粒子
			particle.active = true;

			// 在顶部随机生成粒子
			particle.position.x = randomRange(spawnXMin, spawnXMax);
			particle.position.y = spawnY + randomRange(-0.5f, 0.5f);

			// 设置速度 = 基础速度 + 随机偏移
			particle.velocity = m_config.particles.velocity;
			particle.velocity.x += randomRange(
				-m_config.particles.velocityVariation.x,
				m_config.particles.velocityVariation.x
			);
			particle.velocity.y += randomRange(
				-m_config.particles.velocityVariation.y,
				m_config.particles.velocityVariation.y
			);

			particle.size = randomRange(m_config.particles.sizeMin, m_config.particles.sizeMax);								// 随机大小
			particle.rotation = randomRange(0.0f, 6.28318f);				// 随机初始旋转
			particle.maxLife = m_config.particles.particleLifetime;			// 生命值
			particle.life = particle.maxLife;
			particle.color = m_config.particles.colorStart;					// 初始颜色

			m_poolIndex = (m_poolIndex + 1) % m_particlePool.size();
		}
	}

	WeatherConfig WeatherSystem::getPreset(WeatherType type)
	{
		WeatherConfig config;
		config.type = type;

		switch (type)
		{
		case WeatherType::Rain:
			config.intensity = 1.0f;
			config.particles.spawnRate = 300.0f;
			config.particles.particleLifetime = 2.0f;
			config.particles.velocity = { 0.0f, -12.0f };
			config.particles.velocityVariation = { 0.5f, 2.0f };
			config.particles.sizeMin = 0.02f;
			config.particles.sizeMax = 0.04f;
			config.particles.colorStart = { 0.7f, 0.8f, 1.0f, 0.6f };
			config.particles.colorEnd = { 0.7f, 0.8f, 1.0f, 0.0f };
			config.particles.spawnWidthMultiplier = 1.5f;
			config.particles.spawnHeightOffset = 0.6f;
			break;

		case WeatherType::Snow:
			config.intensity = 1.0f;
			config.particles.spawnRate = 80.0f;
			config.particles.particleLifetime = 8.0f;
			config.particles.velocity = { 0.0f, -1.5f };
			config.particles.velocityVariation = { 0.8f, 0.3f };
			config.particles.sizeMin = 0.04f;
			config.particles.sizeMax = 0.1f;
			config.particles.colorStart = { 1.0f, 1.0f, 1.0f, 0.9f };
			config.particles.colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };
			config.particles.spawnWidthMultiplier = 1.8f;
			config.particles.spawnHeightOffset = 0.7f;
			config.windStrength = 0.2f;
			break;

		case WeatherType::Storm:
			config = getPreset(WeatherType::Rain);
			config.type = WeatherType::Storm;
			config.intensity = 2.0f;
			config.windStrength = 0.5f;
			config.particles.spawnRate = 500.0f;
			config.particles.velocity = { 2.0f, -15.0f };
			break;

		case WeatherType::None:
		default:
			config.particles.spawnRate = 0.0f;
			break;
		}

		return config;
	}

	float WeatherSystem::randomFloat()
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	float WeatherSystem::randomRange(float min, float max)
	{
		return min + randomFloat() * (max - min);
	}

}