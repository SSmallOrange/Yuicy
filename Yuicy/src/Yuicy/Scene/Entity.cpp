#include "pch.h"
#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

}