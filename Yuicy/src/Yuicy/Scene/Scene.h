#pragma once

#include <entt.hpp>

#include "Yuicy/Core/Timestep.h"

class b2World;

namespace Yuicy {

	class Entity;
	class ContactListener;

	class Scene
	{
	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts);

		void OnUpdate(Timestep ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		Entity FindEntityByName(const std::string& name);

		b2World* GetPhysicsWorld() { return m_PhysicsWorld; }

	private:
		// 脚本
		void InitializeScripts();			
		void UpdateScripts(Timestep ts);	
		void DestroyScripts();

		// Lua 脚本
		void InitializeLuaScripts();
		void UpdateLuaScripts(Timestep ts);
		void DestroyLuaScripts();
		void ProcessLuaCollisionCallbacks();
		// 碰撞回调
		void ProcessCollisionCallbacks();
		// 动画
		void UpdateAnimations(Timestep ts);

		void RenderScene();

	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		// 物理系统
		b2World* m_PhysicsWorld = nullptr;
		ContactListener* m_ContactListener = nullptr;

		friend class Entity;
	};
}