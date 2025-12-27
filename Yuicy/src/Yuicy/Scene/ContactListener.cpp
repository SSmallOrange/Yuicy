#include "pch.h"
#include "Yuicy/Scene/ContactListener.h"

#include <box2d/b2_contact.h>
#include <box2d/b2_fixture.h>

namespace Yuicy {

	void ContactListener::BeginContact(b2Contact* contact)
	{
		CollisionInfo info;

		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		// 从 fixture 的 userData 获取实体信息
		// Box2D 的 body 的 userData 存储的是 entt::entity
		info.EntityA = fixtureA->GetBody()->GetUserData().pointer ?
			(void*)fixtureA->GetBody()->GetUserData().pointer : nullptr;
		info.EntityB = fixtureB->GetBody()->GetUserData().pointer ?
			(void*)fixtureB->GetBody()->GetUserData().pointer : nullptr;

		info.IsSensorA = fixtureA->IsSensor();
		info.IsSensorB = fixtureB->IsSensor();

		m_BeginContacts.push_back(info);
	}

	void ContactListener::EndContact(b2Contact* contact)
	{
		CollisionInfo info;

		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();

		info.EntityA = fixtureA->GetBody()->GetUserData().pointer ?
			(void*)fixtureA->GetBody()->GetUserData().pointer : nullptr;
		info.EntityB = fixtureB->GetBody()->GetUserData().pointer ?
			(void*)fixtureB->GetBody()->GetUserData().pointer : nullptr;

		info.IsSensorA = fixtureA->IsSensor();
		info.IsSensorB = fixtureB->IsSensor();

		m_EndContacts.push_back(info);
	}

	void ContactListener::ClearContacts()
	{
		m_BeginContacts.clear();
		m_EndContacts.clear();
	}

}