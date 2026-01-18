#pragma once

#include <glm/glm.hpp>
#include <entt.hpp>
#include <cstdint>

namespace Yuicy {

	// 射线检测结果
	struct RaycastResult2D
	{
		bool hit = false;
		glm::vec2 point = { 0.0f, 0.0f };
		glm::vec2 normal = { 0.0f, 0.0f };
		float fraction = 1.0f;
		entt::entity hitEntity = entt::null;
	};

	// 碰撞查询过滤器
	struct Physics2DFilter
	{
		uint16_t categoryBits = 0xFFFF;
		uint16_t maskBits = 0xFFFF;
	};

}
