#include "pch.h"
#include "WeatherSystem.h"
#include "WeatherPresets.h"
#include "SplashEffect.h"
#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Physics/Physics2D.h"

#include <cstdlib>
#include <ctime>

namespace Yuicy {

	WeatherSystem::WeatherSystem(uint32_t maxParticles)
	{
		m_particlePool.resize(maxParticles);
		m_poolIndex = 0;

		// 初始化物理雨滴池
		m_physicsRaindrops.resize(MAX_PHYSICS_RAINDROPS);
		m_physicsRaindropIndex = 0;

		std::srand(static_cast<unsigned>(std::time(nullptr)));
	}

	void WeatherSystem::SetWeather(WeatherType type, WeatherIntensity intensity)
	{
		m_currentConfig = WeatherPresets::Get(type, intensity);
		m_isTransitioning = false;
	}

	void WeatherSystem::SetWeather(const WeatherConfig& config)
	{
		m_currentConfig = config;
		m_isTransitioning = false;
	}

	void WeatherSystem::TransitionTo(WeatherType type, WeatherIntensity intensity)
	{
		TransitionTo(WeatherPresets::Get(type, intensity));
	}

	void WeatherSystem::TransitionTo(const WeatherConfig& config)
	{
		// 保存当前配置作为过渡起点
		m_previousConfig = m_currentConfig;
		m_targetConfig = config;

		// 开始过渡
		m_isTransitioning = true;
		m_transitionProgress = 0.0f;
		m_transitionDuration = config.transition.duration;
	}

	void WeatherSystem::TransitionTo(const std::string& presetName)
	{
		if (WeatherPresets::HasPreset(presetName))
		{
			TransitionTo(WeatherPresets::GetByName(presetName));
		}
		else
		{
			YUICY_CORE_WARN("Weather preset '{}' not found!", presetName);
		}
	}

	void WeatherSystem::Clear()
	{
		m_currentConfig.type = WeatherType::None;  // 
		m_currentConfig.name = "None";
		m_isTransitioning = false;
	}

	void WeatherSystem::SetWindStrength(float strength)
	{
		m_currentConfig.windStrength = glm::clamp(strength, -1.0f, 1.0f);
	}

	void WeatherSystem::SetIntensity(float intensity)
	{
		m_currentConfig.intensity = glm::max(0.0f, intensity);
	}

	void WeatherSystem::FadeOut(float duration)
	{
		WeatherConfig fadeConfig;
		fadeConfig.type = WeatherType::None;
		fadeConfig.name = "None";
		fadeConfig.transition.duration = duration;
		TransitionTo(fadeConfig);
	}

	void WeatherSystem::OnUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);
		m_globalTime += dt;

		if (m_isTransitioning)
		{
			UpdateTransition(ts);
		}

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
			particle.velocity += m_currentConfig.particles.gravity * dt;

			// 风力影响：windStrength * 基础风速系数
			float windEffect = m_currentConfig.windStrength * 2.0f;
			particle.position.x += windEffect * dt;

			ApplyParticleMotion(particle, dt);

			// 位置
			particle.position += particle.velocity * dt;
			// 旋转
			particle.rotation += m_currentConfig.particles.rotationSpeed * dt;
		}

		// 更新物理雨滴
		if (m_currentConfig.particles.enablePhysics && m_physics2D)
		{
			UpdatePhysicsRaindrops(dt);
		}

		// 更新溅射效果
		SplashEffect::OnUpdate(ts);
	}

	void WeatherSystem::UpdateTransition(Timestep ts)
	{
		float dt = static_cast<float>(ts);
		m_transitionProgress += dt / m_transitionDuration;

		if (m_transitionProgress >= 1.0f)
		{
			// 过渡完成
			m_transitionProgress = 1.0f;
			m_currentConfig = m_targetConfig;
			m_isTransitioning = false;
		}
		else
		{
			// 插值过渡中的参数
			// 使用平滑的缓动函数
			float t = m_transitionProgress;
			float smoothT = t * t * (3.0f - 2.0f * t);  // smoothstep

			// 插值强度
			m_currentConfig.intensity = glm::mix(
				m_previousConfig.intensity,
				m_targetConfig.intensity,
				smoothT
			);

			// 插值风力
			m_currentConfig.windStrength = glm::mix(
				m_previousConfig.windStrength,
				m_targetConfig.windStrength,
				smoothT
			);

			// 插值粒子生成率
			m_currentConfig.particles.spawnRate = glm::mix(
				m_previousConfig.particles.spawnRate,
				m_targetConfig.particles.spawnRate,
				smoothT
			);

			// 插值颜色
			m_currentConfig.particles.colorStart = glm::mix(
				m_previousConfig.particles.colorStart,
				m_targetConfig.particles.colorStart,
				smoothT
			);
			m_currentConfig.particles.colorEnd = glm::mix(
				m_previousConfig.particles.colorEnd,
				m_targetConfig.particles.colorEnd,
				smoothT
			);

			// 在过渡中点切换类型
			if (m_transitionProgress > 0.5f && m_currentConfig.type != m_targetConfig.type)
			{
				m_currentConfig.type = m_targetConfig.type;
				m_currentConfig.name = m_targetConfig.name;
				m_currentConfig.particles.motionType = m_targetConfig.particles.motionType;
			}
		}
	}

	void WeatherSystem::ApplyParticleMotion(Particle& particle, float dt)
	{
		const auto& config = m_currentConfig.particles;
		float time = m_globalTime + particle.phaseOffset;

		switch (config.motionType)
		{
		case ParticleMotion::Swaying:
			// 左右飘动（用于雪花、落叶）
		{
			float sway = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt;
			particle.position.x += sway;
		}
		break;

		case ParticleMotion::Spiral:
			// 螺旋运动
		{
			float spiralX = std::cos(time * config.motionFrequency) * config.motionAmplitude * dt;
			float spiralY = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt * 0.5f;
			particle.position.x += spiralX;
			particle.position.y += spiralY;
		}
		break;

		case ParticleMotion::Random:
			// 随机飘动（用于萤火虫、沙尘）
		{
			// 使用柏林噪声的简化版本
			float noiseX = std::sin(time * config.motionFrequency + particle.phaseOffset * 10.0f);
			float noiseY = std::cos(time * config.motionFrequency * 0.7f + particle.phaseOffset * 7.0f);
			particle.position.x += noiseX * config.motionAmplitude * dt;
			particle.position.y += noiseY * config.motionAmplitude * dt * 0.5f;
		}
		break;

		case ParticleMotion::Rising:
			// 向上飘动（用于火花、萤火虫）
		{
			float rise = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt;
			particle.position.x += rise;
			// 减弱下落速度
			particle.velocity.y *= 0.995f;
		}
		break;

		case ParticleMotion::Linear:
		default:
			// 直线运动，无额外处理
			break;
		}
	}

	void WeatherSystem::OnRender(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		m_lastCameraPos = cameraPos;
		m_lastViewportSize = viewportSize;

		if (m_currentConfig.type != WeatherType::None)
		{
			EmitParticles(1.0f / 60.0f, cameraPos, viewportSize);
		}

		for (const auto& particle : m_particlePool)
		{
			if (!particle.active)
				continue;

			// 计算生命周期进度 [0, 1]
			float lifeProgress = particle.life / particle.maxLife;

			// 颜色插值
			glm::vec4 color = glm::mix(m_currentConfig.particles.colorEnd, m_currentConfig.particles.colorStart, lifeProgress);

			// 透明度衰减
			color.a *= lifeProgress;

			// 过渡期间额外的透明度调整
			if (m_isTransitioning && m_currentConfig.transition.fadeOutOld)
			{
				// 在过渡前半段淡出旧粒子
				if (m_transitionProgress < 0.5f)
				{
					color.a *= 1.0f - m_transitionProgress;
				}
			}

			if (m_currentConfig.particles.texture)
			{
				Renderer2D::DrawRotatedQuad(
					{ particle.position.x, particle.position.y, 0.9f },
					{ particle.size, particle.size },
					particle.rotation,
					m_currentConfig.particles.texture,
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

		// 渲染溅射效果
		SplashEffect::OnRender();
	}

	void WeatherSystem::EmitParticles(float deltaTime, const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		// 计算本帧应该生成的粒子数量
		// spawnRate * intensity = 实际生成速率
		float spawnRate = m_currentConfig.particles.spawnRate * m_currentConfig.intensity;
		m_spawnAccumulator += spawnRate * deltaTime;

		// 生成区域
		float spawnWidth = viewportSize.x * m_currentConfig.particles.spawnWidthMultiplier;
		float spawnY = cameraPos.y + viewportSize.y * m_currentConfig.particles.spawnHeightOffset;
		float spawnXMin = cameraPos.x - spawnWidth * 0.5f;
		float spawnXMax = cameraPos.x + spawnWidth * 0.5f;

		float spawnYMin = spawnY;
		float spawnYMax = spawnY;
		if (m_currentConfig.type == WeatherType::Fireflies)
		{
			spawnYMin = cameraPos.y - viewportSize.y * 0.3f;
			spawnYMax = cameraPos.y + viewportSize.y * 0.3f;
		}

		// 生成累积的粒子
		while (m_spawnAccumulator >= 1.0f)
		{
			m_spawnAccumulator -= 1.0f;
			Particle& particle = m_particlePool[m_poolIndex];

			// 激活粒子
			particle.active = true;

			// 在顶部随机生成粒子
			particle.position.x = RandomRange(spawnXMin, spawnXMax);
			particle.position.y = spawnY + RandomRange(-0.5f, 0.5f);

			// 设置速度 = 基础速度 + 随机偏移
			particle.velocity = m_currentConfig.particles.velocity;
			particle.velocity.x += RandomRange(
				-m_currentConfig.particles.velocityVariation.x,
				m_currentConfig.particles.velocityVariation.x
			);
			particle.velocity.y += RandomRange(
				-m_currentConfig.particles.velocityVariation.y,
				m_currentConfig.particles.velocityVariation.y
			);

			particle.size = RandomRange(m_currentConfig.particles.sizeMin, m_currentConfig.particles.sizeMax);								// 随机大小
			particle.rotation = RandomRange(0.0f, 6.28318f);				// 随机初始旋转
			particle.maxLife = m_currentConfig.particles.particleLifetime;			// 生命值
			particle.life = particle.maxLife;
			particle.color = m_currentConfig.particles.colorStart;					// 初始颜色
			particle.phaseOffset = RandomRange(0.0f, 6.28318f);						// 随机偏移

			m_poolIndex = (m_poolIndex + 1) % m_particlePool.size();
		}
	}

	float WeatherSystem::RandomFloat()
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	float WeatherSystem::RandomRange(float min, float max)
	{
		return min + RandomFloat() * (max - min);
	}

	void WeatherSystem::EmitPhysicsRaindrop(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		PhysicsRaindrop& drop = m_physicsRaindrops[m_physicsRaindropIndex];
		m_physicsRaindropIndex = (m_physicsRaindropIndex + 1) % MAX_PHYSICS_RAINDROPS;

		drop.active = true;

		// 生成位置
		float spawnWidth = viewportSize.x * m_currentConfig.particles.spawnWidthMultiplier;
		float spawnY = cameraPos.y + viewportSize.y * m_currentConfig.particles.spawnHeightOffset;
		float spawnXMin = cameraPos.x - spawnWidth * 0.5f;
		float spawnXMax = cameraPos.x + spawnWidth * 0.5f;

		drop.position.x = RandomRange(spawnXMin, spawnXMax);
		drop.position.y = spawnY;

		// 速度
		drop.velocity = m_currentConfig.particles.velocity;
		drop.velocity.x += RandomRange(-m_currentConfig.particles.velocityVariation.x, m_currentConfig.particles.velocityVariation.x);
		drop.velocity.y += RandomRange(-m_currentConfig.particles.velocityVariation.y, m_currentConfig.particles.velocityVariation.y);
	}

	void WeatherSystem::UpdatePhysicsRaindrops(float dt)
	{
		// 生成新的物理雨滴
		float physicsSpawnRate = m_currentConfig.particles.spawnRate * m_currentConfig.particles.physicsRatio * m_currentConfig.intensity;
		m_physicsSpawnAccumulator += physicsSpawnRate * dt;

		while (m_physicsSpawnAccumulator >= 1.0f)
		{
			m_physicsSpawnAccumulator -= 1.0f;
			EmitPhysicsRaindrop(m_lastCameraPos, m_lastViewportSize);
		}

		// 更新物理雨滴并检测碰撞
		for (auto& drop : m_physicsRaindrops)
		{
			if (!drop.active)
				continue;

			// 保存旧位置
			glm::vec2 oldPos = drop.position;

			// 重力
			drop.velocity += m_currentConfig.particles.gravity * dt;

			// 风力
			float windEffect = m_currentConfig.windStrength * 2.0f;
			drop.position.x += windEffect * dt;

			// 更新位置
			drop.position += drop.velocity * dt;

			// 超出视口检测
			float bottomY = m_lastCameraPos.y - m_lastViewportSize.y * 0.6f;
			if (drop.position.y < bottomY)
			{
				drop.active = false;
				continue;
			}

			if (m_physics2D)
			{
				auto result = m_physics2D->Raycast(oldPos, drop.position);

				if (result.hit)
				{
					// 触发溅射效果
					SplashEffect::Emit(result.point, m_currentConfig.particles.splashConfig);

					// 禁用该雨滴
					drop.active = false;
				}
			}
		}
	}

}
#include "pch.h"
#include "WeatherSystem.h"
#include "WeatherPresets.h"
#include "SplashEffect.h"
#include "Yuicy/Renderer/Renderer2D.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>

#include <cstdlib>
#include <ctime>

namespace Yuicy {

	class RaindropRaycastCallback : public b2RayCastCallback
	{
	public:
		bool hit = false;
		b2Vec2 hitPoint;
		b2Vec2 hitNormal;

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
			const b2Vec2& normal, float fraction) override
		{
			// 忽略 sensor
			if (fixture->IsSensor())
				return -1.0f;

			hit = true;
			hitPoint = point;
			hitNormal = normal;
			return fraction;
		}
	};

	WeatherSystem::WeatherSystem(uint32_t maxParticles)
	{
		m_particlePool.resize(maxParticles);
		m_poolIndex = 0;

		// 初始化物理雨滴池
		m_physicsRaindrops.resize(MAX_PHYSICS_RAINDROPS);
		m_physicsRaindropIndex = 0;

		std::srand(static_cast<unsigned>(std::time(nullptr)));
	}

	void WeatherSystem::SetWeather(WeatherType type, WeatherIntensity intensity)
	{
		m_currentConfig = WeatherPresets::Get(type, intensity);
		m_isTransitioning = false;
	}

	void WeatherSystem::SetWeather(const WeatherConfig& config)
	{
		m_currentConfig = config;
		m_isTransitioning = false;
	}

	void WeatherSystem::TransitionTo(WeatherType type, WeatherIntensity intensity)
	{
		TransitionTo(WeatherPresets::Get(type, intensity));
	}

	void WeatherSystem::TransitionTo(const WeatherConfig& config)
	{
		// 保存当前配置作为过渡起点
		m_previousConfig = m_currentConfig;
		m_targetConfig = config;

		// 开始过渡
		m_isTransitioning = true;
		m_transitionProgress = 0.0f;
		m_transitionDuration = config.transition.duration;
	}

	void WeatherSystem::TransitionTo(const std::string& presetName)
	{
		if (WeatherPresets::HasPreset(presetName))
		{
			TransitionTo(WeatherPresets::GetByName(presetName));
		}
		else
		{
			YUICY_CORE_WARN("Weather preset '{}' not found!", presetName);
		}
	}

	void WeatherSystem::Clear()
	{
		m_currentConfig.type = WeatherType::None;  // 
		m_currentConfig.name = "None";
		m_isTransitioning = false;
	}

	void WeatherSystem::SetWindStrength(float strength)
	{
		m_currentConfig.windStrength = glm::clamp(strength, -1.0f, 1.0f);
	}

	void WeatherSystem::SetIntensity(float intensity)
	{
		m_currentConfig.intensity = glm::max(0.0f, intensity);
	}

	void WeatherSystem::FadeOut(float duration)
	{
		WeatherConfig fadeConfig;
		fadeConfig.type = WeatherType::None;
		fadeConfig.name = "None";
		fadeConfig.transition.duration = duration;
		TransitionTo(fadeConfig);
	}

	void WeatherSystem::OnUpdate(Timestep ts)
	{
		float dt = static_cast<float>(ts);
		m_globalTime += dt;

		if (m_isTransitioning)
		{
			UpdateTransition(ts);
		}

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
			particle.velocity += m_currentConfig.particles.gravity * dt;

			// 风力影响：windStrength * 基础风速系数
			float windEffect = m_currentConfig.windStrength * 2.0f;
			particle.position.x += windEffect * dt;

			ApplyParticleMotion(particle, dt);

			// 位置
			particle.position += particle.velocity * dt;
			// 旋转
			particle.rotation += m_currentConfig.particles.rotationSpeed * dt;
		}

		// 更新物理雨滴
		if (m_currentConfig.particles.enablePhysics && m_physicsWorld)
		{
			UpdatePhysicsRaindrops(dt);
		}

		// 更新溅射效果
		SplashEffect::OnUpdate(ts);
	}

	void WeatherSystem::UpdateTransition(Timestep ts)
	{
		float dt = static_cast<float>(ts);
		m_transitionProgress += dt / m_transitionDuration;

		if (m_transitionProgress >= 1.0f)
		{
			// 过渡完成
			m_transitionProgress = 1.0f;
			m_currentConfig = m_targetConfig;
			m_isTransitioning = false;
		}
		else
		{
			// 插值过渡中的参数
			// 使用平滑的缓动函数
			float t = m_transitionProgress;
			float smoothT = t * t * (3.0f - 2.0f * t);  // smoothstep

			// 插值强度
			m_currentConfig.intensity = glm::mix(
				m_previousConfig.intensity,
				m_targetConfig.intensity,
				smoothT
			);

			// 插值风力
			m_currentConfig.windStrength = glm::mix(
				m_previousConfig.windStrength,
				m_targetConfig.windStrength,
				smoothT
			);

			// 插值粒子生成率
			m_currentConfig.particles.spawnRate = glm::mix(
				m_previousConfig.particles.spawnRate,
				m_targetConfig.particles.spawnRate,
				smoothT
			);

			// 插值颜色
			m_currentConfig.particles.colorStart = glm::mix(
				m_previousConfig.particles.colorStart,
				m_targetConfig.particles.colorStart,
				smoothT
			);
			m_currentConfig.particles.colorEnd = glm::mix(
				m_previousConfig.particles.colorEnd,
				m_targetConfig.particles.colorEnd,
				smoothT
			);

			// 在过渡中点切换类型
			if (m_transitionProgress > 0.5f && m_currentConfig.type != m_targetConfig.type)
			{
				m_currentConfig.type = m_targetConfig.type;
				m_currentConfig.name = m_targetConfig.name;
				m_currentConfig.particles.motionType = m_targetConfig.particles.motionType;
			}
		}
	}

	void WeatherSystem::ApplyParticleMotion(Particle& particle, float dt)
	{
		const auto& config = m_currentConfig.particles;
		float time = m_globalTime + particle.phaseOffset;

		switch (config.motionType)
		{
		case ParticleMotion::Swaying:
			// 左右飘动（用于雪花、落叶）
		{
			float sway = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt;
			particle.position.x += sway;
		}
		break;

		case ParticleMotion::Spiral:
			// 螺旋运动
		{
			float spiralX = std::cos(time * config.motionFrequency) * config.motionAmplitude * dt;
			float spiralY = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt * 0.5f;
			particle.position.x += spiralX;
			particle.position.y += spiralY;
		}
		break;

		case ParticleMotion::Random:
			// 随机飘动（用于萤火虫、沙尘）
		{
			// 使用柏林噪声的简化版本
			float noiseX = std::sin(time * config.motionFrequency + particle.phaseOffset * 10.0f);
			float noiseY = std::cos(time * config.motionFrequency * 0.7f + particle.phaseOffset * 7.0f);
			particle.position.x += noiseX * config.motionAmplitude * dt;
			particle.position.y += noiseY * config.motionAmplitude * dt * 0.5f;
		}
		break;

		case ParticleMotion::Rising:
			// 向上飘动（用于火花、萤火虫）
		{
			float rise = std::sin(time * config.motionFrequency) * config.motionAmplitude * dt;
			particle.position.x += rise;
			// 减弱下落速度
			particle.velocity.y *= 0.995f;
		}
		break;

		case ParticleMotion::Linear:
		default:
			// 直线运动，无额外处理
			break;
		}
	}

	void WeatherSystem::OnRender(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		m_lastCameraPos = cameraPos;
		m_lastViewportSize = viewportSize;

		if (m_currentConfig.type != WeatherType::None)
		{
			EmitParticles(1.0f / 60.0f, cameraPos, viewportSize);
		}

		for (const auto& particle : m_particlePool)
		{
			if (!particle.active)
				continue;

			// 计算生命周期进度 [0, 1]
			float lifeProgress = particle.life / particle.maxLife;

			// 颜色插值
			glm::vec4 color = glm::mix(m_currentConfig.particles.colorEnd, m_currentConfig.particles.colorStart, lifeProgress);

			// 透明度衰减
			color.a *= lifeProgress;

			// 过渡期间额外的透明度调整
			if (m_isTransitioning && m_currentConfig.transition.fadeOutOld)
			{
				// 在过渡前半段淡出旧粒子
				if (m_transitionProgress < 0.5f)
				{
					color.a *= 1.0f - m_transitionProgress;
				}
			}

			if (m_currentConfig.particles.texture)
			{
				Renderer2D::DrawRotatedQuad(
					{ particle.position.x, particle.position.y, 0.9f },
					{ particle.size, particle.size },
					particle.rotation,
					m_currentConfig.particles.texture,
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

		// 渲染溅射效果
		SplashEffect::OnRender();
	}

	void WeatherSystem::EmitParticles(float deltaTime, const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		// 计算本帧应该生成的粒子数量
		// spawnRate * intensity = 实际生成速率
		float spawnRate = m_currentConfig.particles.spawnRate * m_currentConfig.intensity;
		m_spawnAccumulator += spawnRate * deltaTime;

		// 生成区域
		float spawnWidth = viewportSize.x * m_currentConfig.particles.spawnWidthMultiplier;
		float spawnY = cameraPos.y + viewportSize.y * m_currentConfig.particles.spawnHeightOffset;
		float spawnXMin = cameraPos.x - spawnWidth * 0.5f;
		float spawnXMax = cameraPos.x + spawnWidth * 0.5f;

		float spawnYMin = spawnY;
		float spawnYMax = spawnY;
		if (m_currentConfig.type == WeatherType::Fireflies)
		{
			spawnYMin = cameraPos.y - viewportSize.y * 0.3f;
			spawnYMax = cameraPos.y + viewportSize.y * 0.3f;
		}

		// 生成累积的粒子
		while (m_spawnAccumulator >= 1.0f)
		{
			m_spawnAccumulator -= 1.0f;
			Particle& particle = m_particlePool[m_poolIndex];

			// 激活粒子
			particle.active = true;

			// 在顶部随机生成粒子
			particle.position.x = RandomRange(spawnXMin, spawnXMax);
			particle.position.y = spawnY + RandomRange(-0.5f, 0.5f);

			// 设置速度 = 基础速度 + 随机偏移
			particle.velocity = m_currentConfig.particles.velocity;
			particle.velocity.x += RandomRange(
				-m_currentConfig.particles.velocityVariation.x,
				m_currentConfig.particles.velocityVariation.x
			);
			particle.velocity.y += RandomRange(
				-m_currentConfig.particles.velocityVariation.y,
				m_currentConfig.particles.velocityVariation.y
			);

			particle.size = RandomRange(m_currentConfig.particles.sizeMin, m_currentConfig.particles.sizeMax);								// 随机大小
			particle.rotation = RandomRange(0.0f, 6.28318f);				// 随机初始旋转
			particle.maxLife = m_currentConfig.particles.particleLifetime;			// 生命值
			particle.life = particle.maxLife;
			particle.color = m_currentConfig.particles.colorStart;					// 初始颜色
			particle.phaseOffset = RandomRange(0.0f, 6.28318f);						// 随机偏移

			m_poolIndex = (m_poolIndex + 1) % m_particlePool.size();
		}
	}

	float WeatherSystem::RandomFloat()
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	float WeatherSystem::RandomRange(float min, float max)
	{
		return min + RandomFloat() * (max - min);
	}

	void WeatherSystem::EmitPhysicsRaindrop(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		PhysicsRaindrop& drop = m_physicsRaindrops[m_physicsRaindropIndex];
		m_physicsRaindropIndex = (m_physicsRaindropIndex + 1) % MAX_PHYSICS_RAINDROPS;

		drop.active = true;

		// 生成位置
		float spawnWidth = viewportSize.x * m_currentConfig.particles.spawnWidthMultiplier;
		float spawnY = cameraPos.y + viewportSize.y * m_currentConfig.particles.spawnHeightOffset;
		float spawnXMin = cameraPos.x - spawnWidth * 0.5f;
		float spawnXMax = cameraPos.x + spawnWidth * 0.5f;

		drop.position.x = RandomRange(spawnXMin, spawnXMax);
		drop.position.y = spawnY;

		// 速度
		drop.velocity = m_currentConfig.particles.velocity;
		drop.velocity.x += RandomRange(-m_currentConfig.particles.velocityVariation.x, m_currentConfig.particles.velocityVariation.x);
		drop.velocity.y += RandomRange(-m_currentConfig.particles.velocityVariation.y, m_currentConfig.particles.velocityVariation.y);
	}

	void WeatherSystem::UpdatePhysicsRaindrops(float dt)
	{
		// 生成新的物理雨滴
		float physicsSpawnRate = m_currentConfig.particles.spawnRate * m_currentConfig.particles.physicsRatio * m_currentConfig.intensity;
		m_physicsSpawnAccumulator += physicsSpawnRate * dt;

		while (m_physicsSpawnAccumulator >= 1.0f)
		{
			m_physicsSpawnAccumulator -= 1.0f;
			EmitPhysicsRaindrop(m_lastCameraPos, m_lastViewportSize);
		}

		// 更新物理雨滴并检测碰撞
		for (auto& drop : m_physicsRaindrops)
		{
			if (!drop.active)
				continue;

			// 保存旧位置
			glm::vec2 oldPos = drop.position;

			// 重力
			drop.velocity += m_currentConfig.particles.gravity * dt;

			// 风力
			float windEffect = m_currentConfig.windStrength * 2.0f;
			drop.position.x += windEffect * dt;

			// 更新位置
			drop.position += drop.velocity * dt;

			// 超出视口检测
			float bottomY = m_lastCameraPos.y - m_lastViewportSize.y * 0.6f;
			if (drop.position.y < bottomY)
			{
				drop.active = false;
				continue;
			}

			// Box2D Raycast 检测碰撞
			if (m_physicsWorld)
			{
				b2Vec2 p1(oldPos.x, oldPos.y);
				b2Vec2 p2(drop.position.x, drop.position.y);

				RaindropRaycastCallback callback;
				m_physicsWorld->RayCast(&callback, p1, p2);

				if (callback.hit)
				{
					// 触发溅射效果
					glm::vec2 hitPos(callback.hitPoint.x, callback.hitPoint.y);
					SplashEffect::Emit(hitPos, m_currentConfig.particles.splashConfig);

					// 禁用该雨滴
					drop.active = false;
				}
			}
		}
	}

}