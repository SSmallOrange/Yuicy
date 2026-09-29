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

		static bool IsMouseButtonPressed(MouseCode button);
		// static glm::vec2 GetMousePosition();
		static std::pair<float, float> GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}