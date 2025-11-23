#include "pch.h"

#include "ParticleSystem.h"

#include "Renderer2D.h"
#include "Yuicy/Core/Timestep.h"

#include <cstdlib>
#include <glm/glm.hpp>

namespace Yuicy {

	static float RandomFloat()
	{
		return (float)rand() / (float)RAND_MAX; // [0,1)
	}

	ParticleSystem::ParticleSystem(uint32_t maxParticles)
	{
		m_ParticlePool.resize(maxParticles);
		m_PoolIndex = maxParticles - 1;
	}

	void ParticleSystem::OnUpdate(Timestep ts)
	{
		float dt = (float)ts;

		for (auto& particle : m_ParticlePool)
		{
			if (!particle.Active)
				continue;

			if (particle.LifeRemaining <= 0.0f)
			{
				particle.Active = false;
				continue;
			}

			particle.LifeRemaining -= dt;
			particle.Position += particle.Velocity * dt;
			// 简单加一点旋转，纯视觉效果
			particle.Rotation += 0.2f * dt;
		}
	}

	void ParticleSystem::OnRender()
	{
		for (const auto& particle : m_ParticlePool)
		{
			if (!particle.Active)
				continue;

			// life ∈ (0,1]
			float life = particle.LifeRemaining / particle.LifeTime;

			// 颜色插值 & alpha 随时间衰减
			glm::vec4 color = glm::mix(particle.ColorEnd, particle.ColorBegin, life);
			color.a *= life;

			// 尺寸插值
			float size = glm::mix(particle.SizeEnd, particle.SizeBegin, life);

			// 这里直接画有颜色的 quad，不用纹理
			Renderer2D::DrawRotatedQuad(
				{ particle.Position.x, particle.Position.y, 0.5f },
				{ size, size },
				particle.Rotation,
				color
			);
		}
	}

	void ParticleSystem::Emit(const ParticleProps& props)
	{
		// 取池子里当前索引的粒子
		Particle& particle = m_ParticlePool[m_PoolIndex];

		particle.Active = true;

		// 位置
		particle.Position = props.Position;
		// 随机初始旋转
		particle.Rotation = RandomFloat() * 2.0f * 3.1415926f;

		// 速度 = 基础速度 + 随机偏移
		particle.Velocity = props.Velocity;
		particle.Velocity.x += props.VelocityVariation.x * (RandomFloat() - 0.5f);
		particle.Velocity.y += props.VelocityVariation.y * (RandomFloat() - 0.5f);

		// 颜色
		particle.ColorBegin = props.ColorBegin;
		particle.ColorEnd = props.ColorEnd;

		// 寿命
		particle.LifeTime = props.LifeTime;
		particle.LifeRemaining = props.LifeTime;

		// 尺寸 = 基础尺寸 + 随机偏移
		particle.SizeBegin = props.SizeBegin + props.SizeVariation * (RandomFloat() - 0.5f);
		particle.SizeEnd = props.SizeEnd;

		// 池子循环使用
		if (m_PoolIndex == 0)
			m_PoolIndex = (uint32_t)m_ParticlePool.size() - 1;
		else
			--m_PoolIndex;
	}

} // namespace Yuicy
