#pragma once

#include <entt.hpp>
#include <unordered_map>

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Physics/Physics2D.h"

class b2World;

namespace Yuicy {

	class Entity;
	class ContactListener;
	class EditorCamera;

	class Scene : public Asset
	{
	public:
		static AssetType GetStaticType() { return AssetType::Scene; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

		// 场景拷贝
		static Ref<Scene> Copy(Ref<Scene> source);

	public:
		Scene();
		~Scene();

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		Entity CreateChildEntity(Entity parent, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnRuntimeStart();
		void OnRuntimeStop();

		void OnUpdateRuntime(Timestep ts);
		void OnUpdateEditor(Timestep ts, EditorCamera& camera);

		void OnUpdate(Timestep ts);
		void OnViewportResize(uint32_t width, uint32_t height);

		Entity FindEntityByName(const std::string& name);
		Entity FindEntityByUUID(UUID uuid);

		// 父子关系
		void ParentEntity(Entity entity, Entity parent);
		void UnparentEntity(Entity entity, bool convertToWorldSpace = true);

		// 空间坐标转换
		void ConvertToLocalSpace(Entity entity);
		void ConvertToWorldSpace(Entity entity);
		glm::mat4 GetWorldSpaceTransformMatrix(Entity entity);
		TransformComponent GetWorldSpaceTransform(Entity entity);

		// 场景名称
		const std::string& GetName() const { return m_name; }
		void SetName(const std::string& name) { m_name = name; }

		// 物理系统
		b2World* GetPhysicsWorld() { return m_PhysicsWorld; }
		Physics2D& GetPhysics2D() { return m_Physics2D; }

		// 实体遍历
		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}

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
		void RenderScene(EditorCamera& camera);

	private:
		entt::registry m_Registry;
		std::string m_name;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

		// UUID -> Entity 映射
		std::unordered_map<UUID, entt::entity> m_EntityIDMap;

		// 物理系统
		b2World* m_PhysicsWorld = nullptr;
		ContactListener* m_ContactListener = nullptr;
		Physics2D m_Physics2D;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};
}
