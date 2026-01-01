#pragma once

#include <glm/glm.hpp>
#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Texture.h"

namespace Yuicy {

	enum class WeatherType : uint8_t
	{
		None = 0,   // 无天气
		Rain,       // 雨天
		Snow,       // 雪天
		Storm,      // 暴风雨
		Fog,        // 雾天
		Custom      // 自定义
	};

	struct WeatherParticleConfig
	{
		float spawnRate = 100.0f;           // 粒子数量
		float particleLifetime = 3.0f;      // 粒子存活时间（S）

		glm::vec2 velocity = { 0.0f, -5.0f };           // 速度
		glm::vec2 velocityVariation = { 0.5f, 0.5f };   // 速度变化范围
		glm::vec2 gravity = { 0.0f, 0.0f };             // 加速度

		float sizeMin = 0.02f;              // 粒子尺寸
		float sizeMax = 0.05f;
		glm::vec4 colorStart = { 1.0f, 1.0f, 1.0f, 0.8f };  // 初始颜色
		glm::vec4 colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };    // 消亡时颜色

		float spawnWidthMultiplier = 1.5f;  // 生成区域宽度
		float spawnHeightOffset = 0.6f;     // 生成位置在视口顶部偏移

		Ref<Texture2D> texture = nullptr;	// 绘制纹理
	};

	struct WeatherConfig
	{
		WeatherType type = WeatherType::None;
		float intensity = 1.0f;             // 强度系数（影响粒子数量）
		float windStrength = 0.0f;          // 风力 [-1, 1]

		WeatherParticleConfig particles;
	};

}