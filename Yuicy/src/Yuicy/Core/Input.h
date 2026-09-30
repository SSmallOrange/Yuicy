#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Core/MouseCodes.h"

// #include <glm/glm.hpp>

namespace Yuicy {

	class YUICY_API Input 
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		// 主修饰键（快捷键、多选等使用）：Windows / Linux 为 Ctrl，macOS 为 Cmd（Super），左右任一按下即返回 true
		static bool IsPrimaryModifierPressed();

		static bool IsMouseButtonPressed(MouseCode button);
		// static glm::vec2 GetMousePosition();
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}