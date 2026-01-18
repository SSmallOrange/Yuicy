#include "pch.h"
#include "Physics2D.h"

#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>

namespace Yuicy {

	// Raycast 回调
	class Physics2DRaycastCallback : public b2RayCastCallback
	{
	public:
		bool hit = false;
		b2Vec2 hitPoint;
		b2Vec2 hitNormal;
		float hitFraction = 1.0f;
		entt::entity hitEntity = entt::null;
		uint16_t maskBits = 0xFFFF;

		float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
			const b2Vec2& normal, float fraction) override
		{
			// 跳过 sensor (trigger)
			if (fixture->IsSensor())
				return -1.0f;

			// 检查碰撞层
			uint16_t categoryBits = fixture->GetFilterData().categoryBits;
			if ((categoryBits & maskBits) == 0)
				return -1.0f;

			// 记录更近的碰撞
			if (fraction < hitFraction)
			{
				hit = true;
				hitPoint = point;
				hitNormal = normal;
				hitFraction = fraction;

				// 从 body user data 获取 entity
				b2Body* body = fixture->GetBody();
				if (body)
				{
					hitEntity = (entt::entity)body->GetUserData().pointer;
				}
			}

			return fraction;  // 继续查找更近的碰撞
		}
	};

	RaycastResult2D Physics2D::Raycast(const glm::vec2& start, const glm::vec2& end, uint16_t maskBits)
	{
		RaycastResult2D result;

		if (!m_World)
			return result;

		Physics2DRaycastCallback callback;
		callback.maskBits = maskBits;

		b2Vec2 p1(start.x, start.y);
		b2Vec2 p2(end.x, end.y);

		m_World->RayCast(&callback, p1, p2);

		if (callback.hit)
		{
			result.hit = true;
			result.point = { callback.hitPoint.x, callback.hitPoint.y };
			result.normal = { callback.hitNormal.x, callback.hitNormal.y };
			result.fraction = callback.hitFraction;
			result.hitEntity = callback.hitEntity;
		}

		return result;
	}

	bool Physics2D::HasLineOfSight(const glm::vec2& from, const glm::vec2& to, uint16_t maskBits)
	{
		auto result = Raycast(from, to, maskBits);
		return !result.hit;
	}

}
