#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Yuicy {

	struct PostProcessConfig
	{
		bool vignetteEnabled = false;
		float vignetteIntensity = 0.0f;
		float vignetteRadius = 0.8f;

		bool raindropsEnabled = false;
		float raindropsIntensity = 0.5f;
		float raindropsTime = 0.0f;

		bool lightingEnabled = false;
		uint32_t lightMapTextureID = 0;

		std::string sourceName = "Default";
		int priority = 0;
	};

	enum class PostProcessBlendMode
	{
		Replace,    // 完全替换
		Multiply,   // 相乘混合
		Add,        // 相加混合
		Lerp        // 线性插值
	};
}
