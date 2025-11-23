#pragma once

#include <vector>
#include <glm/glm.hpp>

namespace Yuicy {

	class Timestep; // 前向声明，不强依赖头文件

	// 对外暴露的粒子模板属性
	struct ParticleProps
	{
		glm::vec2 Position = { 0.0f, 0.0f };

		glm::vec2 Velocity = { 0.0f, 0.0f };
		glm::vec2 VelocityVariation = { 0.0f, 0.0f };

		glm::vec4 ColorBegin = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 ColorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };

		float SizeBegin = 1.0f;
		float SizeEnd = 0.0f;
		float SizeVariation = 0.0f;

		float LifeTime = 1.0f; // 秒
	};

	// 内部粒子系统
	class ParticleSystem
	{
	public:
		// maxParticles: 粒子池大小
		ParticleSystem(uint32_t maxParticles = 1000);

		// 每帧更新
		void OnUpdate(Timestep ts);

		// 在 Renderer2D::BeginScene / EndScene 之间调用
		void OnRender();

		// 发射一个新粒子（按模板属性生成）
		void Emit(const ParticleProps& particleProps);

	private:
		struct Particle
		{
			glm::vec2 Position;
			glm::vec2 Velocity;

			glm::vec4 ColorBegin, ColorEnd;

			float Rotation = 0.0f;
			float SizeBegin = 1.0f, SizeEnd = 0.0f;

			float LifeTime = 1.0f;
			float LifeRemaining = 0.0f;

			bool Active = false;
		};

		std::vector<Particle> m_ParticlePool;
		uint32_t m_PoolIndex = 0;
	};

} // namespace Yuicy
