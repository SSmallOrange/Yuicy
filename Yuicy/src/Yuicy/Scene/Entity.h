#pragma once
#include "Yuicy/Scene/Scene.h"

#include <entt.hpp>
#include <ranges>

namespace Yuicy {

	class Entity
	{
	public:
		Entity() = default;
		Entity(entt::entity handle, Scene* scene);
		Entity(const Entity& other) = default;

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
			YUICY_CORE_ASSERT(!HasComponent<T>(), "Entity already has component!");
			return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T>
		T& GetComponent()
		{
			YUICY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		const T& GetComponent() const
		{
			YUICY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			return m_Scene->m_Registry.get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			YUICY_CORE_ASSERT(HasComponent<T>(), "Entity does not have component!");
			m_Scene->m_Registry.remove<T>(m_EntityHandle);
		}

		Scene* GetScene() const { return m_Scene; }
		entt::entity GetEntityId() { return m_EntityHandle; }
		UUID GetUUID() { return GetComponent<IDComponent>().ID; }

		TransformComponent& Transform() { return GetComponent<TransformComponent>(); }
		const TransformComponent& Transform() const { return GetComponent<TransformComponent>(); }

		Entity GetParent() const;

		void SetParent(Entity parent)
		{
			Entity currentParent = GetParent();
			if (currentParent == parent)
				return;

			if (currentParent)
				currentParent.RemoveChild(*this);

			SetParentUUID(parent.GetUUID());

			if (parent)
			{
				auto& parentChildren = parent.Children();
				UUID uuid = GetUUID();
				if (std::ranges::find(parentChildren, uuid) == parentChildren.end())
					parentChildren.emplace_back(uuid);
			}
		}

		void SetParentUUID(UUID parent) { GetComponent<RelationshipComponent>().ParentHandle = parent; }
		UUID GetParentUUID() const { return GetComponent<RelationshipComponent>().ParentHandle; }
		std::vector<UUID>& Children() { return GetComponent<RelationshipComponent>().Children; }
		const std::vector<UUID>& Children() const { return GetComponent<RelationshipComponent>().Children; }

		bool RemoveChild(Entity child)
		{
			UUID childId = child.GetUUID();
			std::vector<UUID>& children = Children();
			auto it = std::ranges::find(children, childId);
			if (it != children.end())
			{
				children.erase(it);
				return true;
			}
			return false;
		}

		bool IsAncestorOf(Entity entity) const;
		bool IsDescendantOf(Entity entity) const { return entity.IsAncestorOf(*this); }

		operator bool() const { return m_Scene && m_Scene->m_Registry.valid(m_EntityHandle); }
		operator entt::entity() const { return m_EntityHandle; }
		operator uint32_t() const { return (uint32_t)m_EntityHandle; }

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

	private:
		entt::entity m_EntityHandle{ entt::null };
		Scene* m_Scene = nullptr;

		friend class Scene;
		friend class SceneSerializer;
	};

}