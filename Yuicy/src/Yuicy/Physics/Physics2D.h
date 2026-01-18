#pragma once

#include "Physics2DTypes.h"
#include "Yuicy/Scene/Components.h"

class b2World;

namespace Yuicy {

	class Physics2D
	{
	public:
		Physics2D() = default;
		~Physics2D() = default;

		void SetWorld(b2World* world) { m_World = world; }
		b2World* GetWorld() const { return m_World; }

		// 射线检测，返回第一个碰撞目标
		RaycastResult2D Raycast(const glm::vec2& start, const glm::vec2& end, uint16_t maskBits = 0xFFFF);

		// 检查两点之间是否有清晰视线
		bool HasLineOfSight(const glm::vec2& from, const glm::vec2& to, uint16_t maskBits = CollisionLayer::Ground);

	private:
		b2World* m_World = nullptr;
	};

}
