#pragma once

#include "Entity.h"
#include "Yuicy/Core/Timestep.h"

namespace Yuicy {

	class ScriptableEntity
	{
	public:
		virtual ~ScriptableEntity() = default;

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		const T& GetComponent() const
		{
			return m_Entity.GetComponent<T>();
		}

		template<typename T>
		bool HasComponent() const
		{
			return m_Entity.HasComponent<T>();
		}

		Entity GetEntity() { return m_Entity; }

	protected:
		// 生命周期
		virtual void OnCreate() {}
		virtual void OnDestroy() {}
		virtual void OnUpdate(Timestep ts) {}
		// 事件
		virtual void OnCollisionEnter(Entity other) {}
		virtual void OnCollisionExit(Entity other) {}
		virtual void OnTriggerEnter(Entity other) {}
		virtual void OnTriggerExit(Entity other) {}

	private:
		Entity m_Entity;
		friend class Scene;  // Scene 需要访问 m_Entity 来设置它
	};

}
