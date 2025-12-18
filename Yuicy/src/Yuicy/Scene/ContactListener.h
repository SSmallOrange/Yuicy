#pragma once

#include <box2d/b2_world_callbacks.h>

namespace Yuicy {

	// 碰撞信息结构体
	struct CollisionInfo
	{
		void* EntityA = nullptr;  // 碰撞的实体A
		void* EntityB = nullptr;  // 碰撞的实体B
		bool IsSensorA = false;   // A 是否是触发器
		bool IsSensorB = false;   // B 是否是触发器
	};

	// Box2D 碰撞监听器
	class ContactListener : public b2ContactListener
	{
	public:
		// 碰撞开始 保存碰撞Entity
		void BeginContact(b2Contact* contact) override;

		// 碰撞结束 保存分离Entity
		void EndContact(b2Contact* contact) override;

		// 获取本帧的碰撞事件
		const std::vector<CollisionInfo>& GetBeginContacts() const { return m_BeginContacts; }
		const std::vector<CollisionInfo>& GetEndContacts() const { return m_EndContacts; }

		// 每帧开始时清空碰撞列表
		void ClearContacts();

	private:
		std::vector<CollisionInfo> m_BeginContacts;
		std::vector<CollisionInfo> m_EndContacts;
	};

}
