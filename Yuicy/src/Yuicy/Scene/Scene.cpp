#include "pch.h"
#include "Scene.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Scene/ContactListener.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

namespace Yuicy {

	// 将 Rigidbody2DComponent::BodyType 转换为 b2BodyType
	static b2BodyType Rigidbody2DTypeToBox2DBody(Rigidbody2DComponent::BodyType bodyType)
	{
		switch (bodyType)
		{
		case Rigidbody2DComponent::BodyType::Static:    return b2_staticBody;
		case Rigidbody2DComponent::BodyType::Dynamic:   return b2_dynamicBody;
		case Rigidbody2DComponent::BodyType::Kinematic: return b2_kinematicBody;
		}

		YUICY_CORE_ASSERT(false, "Unknown body type");
		return b2_staticBody;
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		delete m_PhysicsWorld;
		delete m_ContactListener;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;
		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_Registry.destroy(entity.m_EntityHandle);
	}

	void Scene::OnRuntimeStart()
	{
		// 重置累加器
		m_PhysicsAccumulator = 0.0f;

		// 创建 Box2D 物理世界，设置重力
		m_PhysicsWorld = new b2World({ 0.0f, -9.8f });

		// 创建并设置碰撞监听器
		m_ContactListener = new ContactListener();
		m_PhysicsWorld->SetContactListener(m_ContactListener);

		// 为所有拥有 Rigidbody2DComponent 的实体创建 Box2D 刚体
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			// 创建刚体定义
			b2BodyDef bodyDef;
			bodyDef.type = Rigidbody2DTypeToBox2DBody(rb2d.Type);
			bodyDef.position.Set(transform.Translation.x, transform.Translation.y);
			bodyDef.angle = transform.Rotation.z;

			// 创建刚体
			b2Body* body = m_PhysicsWorld->CreateBody(&bodyDef);
			body->SetFixedRotation(rb2d.FixedRotation);

			// 存储实体 ID 到 body 的 userData，用于碰撞回调时识别实体
			body->GetUserData().pointer = (uintptr_t)e;

			rb2d.RuntimeBody = body;

			// 如果有矩形碰撞体，添加夹具
			if (entity.HasComponent<BoxCollider2DComponent>())
			{
				auto& bc2d = entity.GetComponent<BoxCollider2DComponent>();

				b2PolygonShape boxShape;
				boxShape.SetAsBox(
					bc2d.Size.x * transform.Scale.x,
					bc2d.Size.y * transform.Scale.y,
					b2Vec2(bc2d.Offset.x, bc2d.Offset.y),
					0.0f
				);

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &boxShape;
				fixtureDef.density = bc2d.Density;
				fixtureDef.friction = bc2d.Friction;
				fixtureDef.restitution = bc2d.Restitution;
				fixtureDef.restitutionThreshold = bc2d.RestitutionThreshold;

				// 碰撞过滤
				fixtureDef.filter.categoryBits = bc2d.CategoryBits;
				fixtureDef.filter.maskBits = bc2d.MaskBits;

				// 触发器
				fixtureDef.isSensor = bc2d.IsTrigger;

				// 保存 fixture 指针
				bc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			}

			// 如果有圆形碰撞体，添加夹具
			if (entity.HasComponent<CircleCollider2DComponent>())
			{
				auto& cc2d = entity.GetComponent<CircleCollider2DComponent>();

				b2CircleShape circleShape;
				circleShape.m_p.Set(cc2d.Offset.x, cc2d.Offset.y);
				circleShape.m_radius = transform.Scale.x * cc2d.Radius;

				b2FixtureDef fixtureDef;
				fixtureDef.shape = &circleShape;
				fixtureDef.density = cc2d.Density;
				fixtureDef.friction = cc2d.Friction;
				fixtureDef.restitution = cc2d.Restitution;
				fixtureDef.restitutionThreshold = cc2d.RestitutionThreshold;

				// 碰撞过滤
				fixtureDef.filter.categoryBits = cc2d.CategoryBits;
				fixtureDef.filter.maskBits = cc2d.MaskBits;

				// 触发器
				fixtureDef.isSensor = cc2d.IsTrigger;

				// 保存 fixture 指针
				cc2d.RuntimeFixture = body->CreateFixture(&fixtureDef);
			}
		}
	}

	void Scene::OnRuntimeStop()
	{
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			auto& rb2d = m_Registry.get<Rigidbody2DComponent>(e);
			rb2d.RuntimeBody = nullptr;

			// 清空 BoxCollider2D 的 RuntimeFixture
			if (m_Registry.all_of<BoxCollider2DComponent>(e))
			{
				auto& bc2d = m_Registry.get<BoxCollider2DComponent>(e);
				bc2d.RuntimeFixture = nullptr;
			}

			// 清空 CircleCollider2D 的 RuntimeFixture
			if (m_Registry.all_of<CircleCollider2DComponent>(e))
			{
				auto& cc2d = m_Registry.get<CircleCollider2DComponent>(e);
				cc2d.RuntimeFixture = nullptr;
			}
		}

		delete m_ContactListener;
		m_ContactListener = nullptr;

		delete m_PhysicsWorld;
		m_PhysicsWorld = nullptr;
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		// P0: 固定时间步长物理模拟（accumulator 模式）
		if (m_PhysicsWorld)
		{
			// 清空本帧碰撞事件
			m_ContactListener->ClearContacts();

			// Box2D迭代器参数
			const int32_t velocityIterations = 6;
			const int32_t positionIterations = 2;

			// 累加实际时间
			m_PhysicsAccumulator += ts;

			// 以固定步长模拟物理
			while (m_PhysicsAccumulator >= m_PhysicsTimeStep)
			{
				// Transform更新，BeginContact、EndContact等回调触发
				m_PhysicsWorld->Step(m_PhysicsTimeStep, velocityIterations, positionIterations);
				m_PhysicsAccumulator -= m_PhysicsTimeStep;
			}

			// 将物理模拟结果同步回 TransformComponent
			auto view = m_Registry.view<Rigidbody2DComponent>();
			for (auto e : view)
			{
				Entity entity = { e, this };
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

				b2Body* body = (b2Body*)rb2d.RuntimeBody;
				if (body)
				{
					const auto& position = body->GetPosition();
					transform.Translation.x = position.x;
					transform.Translation.y = position.y;
					transform.Rotation.z = body->GetAngle();
				}
			}

			// TODO: 处理碰撞回调事件
		}

		// 渲染场景
		RenderScene();
	}

	void Scene::OnUpdateEditor(Timestep ts)
	{
		// 编辑器模式：只渲染，不进行物理模拟
		RenderScene();
	}

	void Scene::OnUpdate(Timestep ts)
	{
		OnUpdateRuntime(ts);
	}

	void Scene::RenderScene()
	{
		Camera* mainCamera = nullptr;
		glm::mat4 cameraTransform;
		{
			auto view = m_Registry.view<TransformComponent, CameraComponent>();
			for (auto entity : view)
			{
				auto [transform, camera] = view.get<TransformComponent, CameraComponent>(entity);

				if (camera.Primary)
				{
					mainCamera = &camera.Camera;
					cameraTransform = transform.GetTransform();
					break;
				}
			}
		}

		if (mainCamera)
		{
			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			// TODO: 按 SortingOrder 排序
			
			// 获取所有需要渲染的精灵
			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);

				if (sprite.SubTexture)
				{
					Renderer2D::DrawSprite(transform.GetTransform(), 
						sprite.SubTexture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY);
				}
				else if (sprite.Texture)
				{
					Renderer2D::DrawSprite(transform.GetTransform(),
						sprite.Texture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY);
				}
				else
				{
					Renderer2D::DrawQuad(transform.GetTransform(), sprite.Color);
				}
			}

			Renderer2D::EndScene();
		}
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		m_ViewportWidth = width;
		m_ViewportHeight = height;

		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}
}