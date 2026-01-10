#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Yuicy {

	struct PostProcessConfig
	{
		glm::vec4 ambientTint = { 1.0f, 1.0f, 1.0f, 1.0f };  // 环境色
		float brightness = 1.0f;		// 亮度
		float contrast = 1.0f;			// 对比度
		float saturation = 1.0f;		// 饱和度

		bool fogEnabled = false;
		glm::vec4 fogColor = { 0.7f, 0.75f, 0.8f, 1.0f };	// 雾气颜色
		float fogDensity = 0.0f;							// 雾气密度

		bool vignetteEnabled = false;		// 四周变暗
		float vignetteIntensity = 0.0f;		// 强度
		float vignetteRadius = 0.8f;		// 半径

		bool flashEnabled = false;						// 闪光
		float flashIntensity = 0.0f;					// 强度
		glm::vec3 flashColor = { 1.0f, 1.0f, 1.0f };	// 颜色

		bool raindropsEnabled = false;			// 屏幕雨滴
		float raindropsIntensity = 0.5f;		// 雨滴强度 0~1
		float raindropsTime = 0.0f;				// 时间

		std::string sourceName = "Default";
		int priority = 0;                     // 优先级
	};

	enum class PostProcessBlendMode
	{
		Replace,    // 完全替换
		Multiply,   // 相乘混合
		Add,        // 相加混合
		Lerp        // 线性插值
	};
}
