#include "pch.h"
#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	Entity::Entity(entt::entity handle, Scene* scene)
		: m_EntityHandle(handle), m_Scene(scene)
	{
	}

	Entity Entity::GetParent() const
	{
		return m_Scene->FindEntityByUUID(GetParentUUID());
	}

	bool Entity::IsAncestorOf(Entity entity) const
	{
		const auto& children = Children();

		if (children.empty())
			return false;

		for (UUID child : children)
		{
			if (child == entity.GetUUID())
				return true;
		}

		for (UUID child : children)
		{
			Entity childEntity = m_Scene->FindEntityByUUID(child);
			if (childEntity && childEntity.IsAncestorOf(entity))
				return true;
		}

		return false;
	}

}