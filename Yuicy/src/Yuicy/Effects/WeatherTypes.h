#pragma once

#include <glm/glm.hpp>
#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Effects/PostProcessTypes.h"

namespace Yuicy {

	enum class WeatherType : uint8_t
	{
		None = 0,		// 无天气
		Rain,			// 雨天
		Snow,			// 雪天
		Storm,			// 暴风雨
		Fog,			// 雾天
		Drizzle,        // 毛毛雨
		Blizzard,       // 暴风雪
		Sandstorm,      // 沙尘暴
		Leaves,         // 落叶
		Fireflies,      // 萤火虫
		Custom			// 自定义
	};

	enum class WeatherIntensity : uint8_t
	{
		Light = 0,      // 轻微
		Normal,         // 正常
		Heavy,          // 强烈
		Extreme         // 极端
	};

	enum class ParticleMotion : uint8_t
	{
		Linear = 0,     // 直线下落
		Swaying,        // 左右飘动
		Spiral,         // 螺旋运动
		Random,         // 随机飘动
		Rising          // 向上飘动
	};

	struct WeatherParticleConfig
	{
		float spawnRate = 100.0f;           // 粒子数量
		float particleLifetime = 3.0f;      // 粒子存活时间（S）

		glm::vec2 velocity = { 0.0f, -5.0f };           // 速度
		glm::vec2 velocityVariation = { 0.5f, 0.5f };   // 速度变化范围
		glm::vec2 gravity = { 0.0f, 0.0f };             // 加速度
		ParticleMotion motionType = ParticleMotion::Linear;
		float motionFrequency = 5.0f;       // 飘动频率（用于 Swaying/Spiral）
		float motionAmplitude = 0.3f;       // 飘动幅度

		float sizeMin = 0.02f;              // 粒子尺寸
		float sizeMax = 0.05f;
		glm::vec4 colorStart = { 1.0f, 1.0f, 1.0f, 0.8f };  // 初始颜色
		glm::vec4 colorEnd = { 1.0f, 1.0f, 1.0f, 0.0f };    // 消亡时颜色
		float rotationSpeed = 0.0f;							// 旋转速度

		float spawnWidthMultiplier = 1.5f;  // 生成区域宽度
		float spawnHeightOffset = 0.6f;     // 生成位置在视口顶部偏移

		Ref<Texture2D> texture = nullptr;	// 绘制纹理
	};

	struct WeatherTransitionConfig
	{
		bool enabled = true;                // 开启过渡
		float duration = 2.0f;              // 过渡时长（S）
		bool fadeOutOld = true;             // 旧天气是否淡出
	};

	struct WeatherConfig
	{
		WeatherType type = WeatherType::None;
		float intensity = 1.0f;             // 强度系数（影响粒子数量）
		float windStrength = 0.0f;          // 风力 [-1, 1]

		WeatherParticleConfig particles;
		WeatherTransitionConfig transition;
		PostProcessConfig postProcess;

		std::string name = "None";
	};

}