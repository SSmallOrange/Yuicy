#pragma once

#include <entt.hpp>

#include "Yuicy/Core/Timestep.h"

class b2World;

namespace Yuicy {

	class Entity;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();  // 运行时开始（初始化物理世界）
		void OnRuntimeStop();   // 运行时停止（销毁物理世界）

		void OnUpdateRuntime(Timestep ts);  // 运行时更新（包含物理模拟）
		void OnUpdateEditor(Timestep ts);   // 编辑器更新（不包含物理）

		void OnUpdate(Timestep ts);  // 保留原有接口
		void OnViewportResize(uint32_t width, uint32_t height);

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		b2World* m_PhysicsWorld = nullptr;  // Box2D 物理世界

		friend class Entity;
	};

}