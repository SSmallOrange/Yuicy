#include "pch.h"
#include "Scene.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Renderer/Renderer2D.h"

#include <glm/glm.hpp>

namespace Yuicy {

	static void DoMath(const glm::mat4& transform)
	{

	}

	static void OnTransformConstruct(entt::registry& registry, entt::entity entity)
	{

	}

	Scene::Scene()
	{
#if ENTT_EXAMPLE_CODE
		entt::entity entity = m_Registry.create();
		m_Registry.emplace<TransformComponent>(entity, glm::mat4(1.0f));

		// on_construct<组件类型>() 返回一个信号，当该类型组件被创建时触发
		// connect<&函数>() 连接一个回调函数
		// 这样每当有 TransformComponent 被添加时，OnTransformConstruct 函数会自动被调用
		m_Registry.on_construct<TransformComponent>().connect<&OnTransformConstruct>();

		// has<组件类型>(实体) 检查实体是否拥有某个组件，返回 bool
		// get<组件类型>(实体) 获取实体的组件引用，可以读写
		// 注意：如果实体没有该组件就调用 get 会出错，所以先用 has 检查
		if (m_Registry.all_of<TransformComponent>(entity))
			TransformComponent& transform = m_Registry.get<TransformComponent>(entity);

		// view<组件类型...>() 返回一个视图，可以遍历拥有指定组件的所有实体
		auto view = m_Registry.view<TransformComponent>();
		for (auto entity : view)
		{
			TransformComponent& transform = view.get<TransformComponent>(entity);
		}

		// group<Owned...>(get_t<Get...>, exclude_t<Exclude...>) 创建一个组，允许更复杂的查询，比视图更高效
		// Owned 主组件 get<...> 附加组件 exclude<...> 排除组件
		// 获取同时拥有 TransformComponent 和 MeshComponent 的实体
		// Owned会被entt重新排列（改变内存布局），但只能被一个Group拥有，Get组件不会改变内存布局但可以共享
// 		auto group = m_Registry.group<TransformComponent>(entt::get<MeshComponent>);
// 		for (auto entity : group)
// 		{
// 			auto& [transform, mesh] = group.get<TransformComponent, MeshComponent>(entity);
// 		}
#endif
	}

	Scene::~Scene()
	{
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::OnUpdate(Timestep ts)
	{
		// Render 2D
		Camera* mainCamera = nullptr;
		glm::mat4* cameraTransform = nullptr;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				const auto& [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = &transform.Transform;
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, *cameraTransform);

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				const auto& [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				Renderer2D::DrawQuad(transform, sprite.Color);
			}

			Renderer2D::EndScene();
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}

	}
}