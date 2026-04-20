#include "pch.h"
#include "Scene.h"

#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Renderer/RenderCommand.h"
#include "Yuicy/Scene/ContactListener.h"
#include "Yuicy/Scene/ScriptableEntity.h"

#include <glm/glm.hpp>

// Box2D
#include <box2d/b2_world.h>
#include <box2d/b2_body.h>
#include <box2d/b2_fixture.h>
#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>

#include "Yuicy/Scripting/LuaScriptEngine.h"

namespace Yuicy {

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

	template<typename T>
	static void CopyComponent(entt::registry& dstRegistry, entt::registry& srcRegistry, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		auto srcEntities = srcRegistry.view<T>();
		for (auto srcEntity : srcEntities)
		{
			entt::entity dstEntity = enttMap.at(srcRegistry.get<IDComponent>(srcEntity).ID);
			auto& srcComponent = srcRegistry.get<T>(srcEntity);
			dstRegistry.emplace_or_replace<T>(dstEntity, srcComponent);
		}
	}

	template<typename T>
	static void CopyComponentIfExists(entt::entity dst, entt::registry& dstRegistry, entt::entity src, entt::registry& srcRegistry)
	{
		if (srcRegistry.all_of<T>(src))
		{
			auto& srcComponent = srcRegistry.get<T>(src);
			dstRegistry.emplace_or_replace<T>(dst, srcComponent);
		}
	}

	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		delete m_PhysicsWorld;
		delete m_ContactListener;
	}

	Ref<Scene> Scene::Copy(Ref<Scene> source)
	{
		if (!source)
			return {};

		Ref<Scene> newScene = CreateRef<Scene>();
		newScene->m_name = source->m_name;
		newScene->m_ViewportWidth = source->m_ViewportWidth;
		newScene->m_ViewportHeight = source->m_ViewportHeight;

		auto& srcRegistry = source->m_Registry;
		auto& dstRegistry = newScene->m_Registry;

		std::unordered_map<UUID, entt::entity> enttMap;

		auto idView = srcRegistry.view<IDComponent>();
		for (auto srcEntity : idView)
		{
			UUID uuid = srcRegistry.get<IDComponent>(srcEntity).ID;
			const auto& name = srcRegistry.get<TagComponent>(srcEntity).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = newEntity.m_EntityHandle;
		}

		CopyComponent<TagComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<TransformComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<RelationshipComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<SpriteRendererComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<CameraComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<NativeScriptComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<LuaScriptComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<AnimationComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<Rigidbody2DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<BoxCollider2DComponent>(dstRegistry, srcRegistry, enttMap);
		CopyComponent<CircleCollider2DComponent>(dstRegistry, srcRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateChildEntity({}, name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>().ID = uuid;						// UUID
		entity.AddComponent<TransformComponent>();							// Transform
		entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);  // Tag
		entity.AddComponent<RelationshipComponent>();						// Relationship

		m_EntityIDMap[uuid] = entity.m_EntityHandle;
		return entity;
	}

	Entity Scene::CreateChildEntity(Entity parent, const std::string& name)
	{
		Entity entity = CreateEntityWithUUID(UUID(), name);

		if (parent)
			entity.SetParent(parent);

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (!entity)
			return;

		// 递归销毁子实体
		if (entity.HasComponent<RelationshipComponent>())
		{
			auto children = entity.Children();
			for (auto childId : children)
			{
				Entity child = FindEntityByUUID(childId);
				if (child)
					DestroyEntity(child);
			}

			// 移除自己
			Entity parent = entity.GetParent();
			if (parent)
				parent.RemoveChild(entity);
		}

		// 从 UUID 映射中移除
		if (entity.HasComponent<IDComponent>())
			m_EntityIDMap.erase(entity.GetComponent<IDComponent>().ID);

		m_Registry.destroy(entity.m_EntityHandle);
	}

	void Scene::ParentEntity(Entity entity, Entity parent)
	{
		if (parent.IsDescendantOf(entity))
		{
			UnparentEntity(parent);

			Entity newParent = FindEntityByUUID(entity.GetParentUUID());
			if (newParent)
			{
				UnparentEntity(entity);
				ParentEntity(parent, newParent);
			}
		}
		else
		{
			Entity previousParent = FindEntityByUUID(entity.GetParentUUID());
			if (previousParent)
				UnparentEntity(entity);
		}

		entity.SetParentUUID(parent.GetUUID());
		parent.Children().push_back(entity.GetUUID());

		ConvertToLocalSpace(entity);
	}

	void Scene::UnparentEntity(Entity entity, bool convertToWorldSpace)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());
		if (!parent)
			return;

		std::erase(parent.Children(), entity.GetUUID());

		if (convertToWorldSpace)
			ConvertToWorldSpace(entity);

		entity.SetParentUUID(0);
	}

	void Scene::ConvertToLocalSpace(Entity entity)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());
		if (!parent)
			return;

		auto& transform = entity.Transform();
		glm::mat4 parentTransform = GetWorldSpaceTransformMatrix(parent);
		glm::mat4 localTransform = glm::inverse(parentTransform) * transform.GetTransform();
		transform.SetTransform(localTransform);
	}

	void Scene::ConvertToWorldSpace(Entity entity)
	{
		Entity parent = FindEntityByUUID(entity.GetParentUUID());
		if (!parent)
			return;

		glm::mat4 worldTransform = GetWorldSpaceTransformMatrix(entity);
		auto& entityTransform = entity.Transform();
		entityTransform.SetTransform(worldTransform);
	}

	glm::mat4 Scene::GetWorldSpaceTransformMatrix(Entity entity)
	{
		glm::mat4 transform(1.0f);

		Entity parent = FindEntityByUUID(entity.GetParentUUID());
		if (parent)
			transform = GetWorldSpaceTransformMatrix(parent);

		return transform * entity.Transform().GetTransform();
	}

	TransformComponent Scene::GetWorldSpaceTransform(Entity entity)
	{
		glm::mat4 transform = GetWorldSpaceTransformMatrix(entity);
		TransformComponent transformComponent;
		transformComponent.SetTransform(transform);
		return transformComponent;
	}

	void Scene::InitializeScripts()
	{
		// 遍历所有拥有 NativeScriptComponent 的实体
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto e : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(e);

			if (nsc.InstantiateScript && !nsc.Instance)
			{
				nsc.Instance = nsc.InstantiateScript();
				// 设置脚本的实体引用
				nsc.Instance->m_Entity = Entity{ e, this };
				nsc.Instance->OnCreate();
			}
		}
	}

	void Scene::UpdateScripts(Timestep ts)
	{
		// 遍历所有脚本并调用 OnUpdate
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto e : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(e);

			if (nsc.Instance)
			{
				nsc.Instance->OnUpdate(ts);
			}
		}
	}

	void Scene::DestroyScripts()
	{
		// 遍历所有脚本并销毁
		auto view = m_Registry.view<NativeScriptComponent>();
		for (auto e : view)
		{
			auto& nsc = view.get<NativeScriptComponent>(e);

			if (nsc.Instance)
			{
				// 调用 OnDestroy 生命周期方法
				nsc.Instance->OnDestroy();

				// 销毁实例
				if (nsc.DestroyScript)
					nsc.DestroyScript(&nsc);
			}
		}
	}

	void Scene::ProcessCollisionCallbacks()
	{
		if (!m_ContactListener)
			return;

		// 处理碰撞开始事件
		for (const auto& contact : m_ContactListener->GetBeginContacts())
		{
			entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(contact.EntityA));
			entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(contact.EntityB));

			// 如果实体 A 有脚本，通知它
			if (m_Registry.valid(entityA) && m_Registry.all_of<NativeScriptComponent>(entityA))
			{
				auto& nsc = m_Registry.get<NativeScriptComponent>(entityA);
				if (nsc.Instance)
				{
					Entity otherEntity{ entityB, this };
					if (contact.IsSensorA || contact.IsSensorB)
						nsc.Instance->OnTriggerEnter(otherEntity);
					else
						nsc.Instance->OnCollisionEnter(otherEntity);
				}
			}

			// 如果实体 B 有脚本，通知它
			if (m_Registry.valid(entityB) && m_Registry.all_of<NativeScriptComponent>(entityB))
			{
				auto& nsc = m_Registry.get<NativeScriptComponent>(entityB);
				if (nsc.Instance)
				{
					Entity otherEntity{ entityA, this };
					if (contact.IsSensorA || contact.IsSensorB)
						nsc.Instance->OnTriggerEnter(otherEntity);
					else
						nsc.Instance->OnCollisionEnter(otherEntity);
				}
			}
		}

		// 处理碰撞结束事件
		for (const auto& contact : m_ContactListener->GetEndContacts())
		{
			entt::entity entityA = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(contact.EntityA));
			entt::entity entityB = static_cast<entt::entity>(reinterpret_cast<uintptr_t>(contact.EntityB));

			if (m_Registry.valid(entityA) && m_Registry.all_of<NativeScriptComponent>(entityA))
			{
				auto& nsc = m_Registry.get<NativeScriptComponent>(entityA);
				if (nsc.Instance)
				{
					Entity otherEntity{ entityB, this };
					if (contact.IsSensorA || contact.IsSensorB)
						nsc.Instance->OnTriggerExit(otherEntity);
					else
						nsc.Instance->OnCollisionExit(otherEntity);
				}
			}

			if (m_Registry.valid(entityB) && m_Registry.all_of<NativeScriptComponent>(entityB))
			{
				auto& nsc = m_Registry.get<NativeScriptComponent>(entityB);
				if (nsc.Instance)
				{
					Entity otherEntity{ entityA, this };
					if (contact.IsSensorA || contact.IsSensorB)
						nsc.Instance->OnTriggerExit(otherEntity);
					else
						nsc.Instance->OnCollisionExit(otherEntity);
				}
			}
		}
	}

	void Scene::UpdateAnimations(Timestep ts)
	{
		auto view = m_Registry.view<AnimationComponent, SpriteRendererComponent>();

		for (auto entity : view)
		{
			auto& anim = view.get<AnimationComponent>(entity);
			auto& sprite = view.get<SpriteRendererComponent>(entity);

			// 获取当前动画剪辑
			AnimationClip* clip = anim.GetCurrentClip();
			if (!clip || clip->Frames.empty())
				continue;

			// 如果正在播放且未完成，更新帧计时
			if (anim.State.Playing && !anim.State.Finished)
			{
				// 累加时间
				anim.State.Timer += ts;

				// 检查是否需要切换到下一帧
				while (anim.State.Timer >= clip->FrameDuration)
				{
					anim.State.Timer -= clip->FrameDuration;
					anim.State.CurrentFrame++;

					// 检查是否播放完成
					if (anim.State.CurrentFrame >= static_cast<int>(clip->Frames.size()))
					{
						if (clip->Loop)  // 循环播放
						{
							anim.State.CurrentFrame = 0;
						}
						else
						{
							// 非循环：停在最后一帧
							anim.State.CurrentFrame = static_cast<int>(clip->Frames.size()) - 1;
							anim.State.Finished = true;
							anim.State.Playing = false;
							break;
						}
					}
				}
			}

			sprite.SubTexture = anim.GetCurrentFrame();
		}
	}

	void Scene::InitializePhysicsWorld()
	{
		// 创建 Box2D 物理世界，设置重力
		m_PhysicsWorld = new b2World({ 0.0f, -9.8f });

		// 创建并设置碰撞监听器
		m_ContactListener = new ContactListener();
		m_PhysicsWorld->SetContactListener(m_ContactListener);

		// 设置 Physics2D 系统的 world
		m_Physics2D.SetWorld(m_PhysicsWorld);

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

	void Scene::DestroyPhysicsWorld()
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

	void Scene::StepPhysicsWorld(Timestep ts)
	{
		if (!m_PhysicsWorld)
			return;

		// 清空本帧碰撞事件
		m_ContactListener->ClearContacts();

		// Box2D迭代器参数
		const int32_t velocityIterations = 6;
		const int32_t positionIterations = 2;

		// 使用帧时间作为物理步长
		float physicsStep = std::min((float)ts, 1.0f / 30.0f);
		// float physicsStep = 1 / (float)60;
		// YUICY_CORE_INFO("dt = {:.4f}s, FPS = {:.0f}", (float)ts, 1.0f / (float)ts);
		m_PhysicsWorld->Step(physicsStep, velocityIterations, positionIterations);

		// 将物理模拟结果同步回 TransformComponent
		auto view = m_Registry.view<Rigidbody2DComponent>();
		for (auto e : view)
		{
			Entity entity = { e, this };
			auto& transform = entity.GetComponent<TransformComponent>();
			auto& rb2d = entity.GetComponent<Rigidbody2DComponent>();

			b2Body* body = static_cast<b2Body*>(rb2d.RuntimeBody);
			if (body)
			{
				const auto& position = body->GetPosition();
				transform.Translation.x = position.x;
				transform.Translation.y = position.y;
				transform.Rotation.z = body->GetAngle();
			}
		}
	}

	void Scene::OnRuntimeStart()
	{
		InitializePhysicsWorld();
		InitializeScripts();
		InitializeLuaScripts();
	}

	void Scene::OnRuntimeStop()
	{
		DestroyScripts();
		DestroyLuaScripts();
		DestroyPhysicsWorld();
	}

	void Scene::OnSimulationStart()
	{
		InitializePhysicsWorld();
	}

	void Scene::OnSimulationStop()
	{
		// Simulate 模式：清理物理世界，不销毁脚本
		DestroyPhysicsWorld();
	}

	void Scene::OnUpdateRuntime(Timestep ts)
	{
		UpdateScripts(ts);
		UpdateLuaScripts(ts);
		UpdateAnimations(ts);

		StepPhysicsWorld(ts);

		if (m_PhysicsWorld)
		{
			// 原生脚本碰撞回调
			ProcessCollisionCallbacks();
			// Lua脚本碰撞回调
			ProcessLuaCollisionCallbacks();
		}

		// 渲染场景
		RenderScene();
	}

	void Scene::OnUpdateSimulation(Timestep ts, EditorCamera& camera)
	{
		// Simulate 模式：物理更新 + 编辑器相机渲染，不运行脚本
		UpdateAnimations(ts);
		StepPhysicsWorld(ts);
		RenderScene(camera);
	}

	void Scene::OnUpdateEditor(Timestep ts, EditorCamera& camera)
	{
		// 编辑器模式：使用独立的 EditorCamera 渲染，不进行物理模拟
		RenderScene(camera);
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
					cameraTransform = GetWorldSpaceTransformMatrix({ entity, this });
					break;
				}
			}
		}

		if (mainCamera)
		{
			// 禁用2D渲染的深度测试（使用画家算法和排序顺序）
			RenderCommand::SetDepthTest(false);

			Renderer2D::BeginScene(*mainCamera, cameraTransform);

			struct SpriteRenderData
			{
				glm::mat4 Transform;
				SpriteRendererComponent* Sprite;
				int EntityID;
			};
			std::vector<SpriteRenderData> renderQueue;

			auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
			renderQueue.reserve(group.size());

			for (auto entity : group)
			{
				auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
				renderQueue.push_back({ GetWorldSpaceTransformMatrix({ entity, this }), &sprite, (int)entity });
			}

			std::ranges::sort(renderQueue, {}, [](const SpriteRenderData& d) {
				return d.Sprite->SortingOrder;
			});

			for (const auto& data : renderQueue)
			{
				const auto& sprite = *data.Sprite;

				if (sprite.SubTexture)
				{
					Renderer2D::DrawSprite(data.Transform, 
						sprite.SubTexture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY, data.EntityID);
				}
				else if (sprite.TextureHandle != 0)
				{
					Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.TextureHandle);
					if (texture)
						Renderer2D::DrawSprite(data.Transform,
							texture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY, data.EntityID);
					else
						Renderer2D::DrawQuad(data.Transform, sprite.Color, data.EntityID);
				}
				else
				{
					Renderer2D::DrawQuad(data.Transform, sprite.Color, data.EntityID);
				}
			}

			Renderer2D::EndScene();

			RenderCommand::SetDepthTest(true);
		}
	}

	void Scene::RenderScene(EditorCamera& camera)
	{
		// 禁用2D渲染的深度测试
		RenderCommand::SetDepthTest(false);

		Renderer2D::BeginScene(camera);

		struct SpriteRenderData
		{
			glm::mat4 Transform;
			SpriteRendererComponent* Sprite;
			int EntityID;
		};
		std::vector<SpriteRenderData> renderQueue;

		auto group = m_Registry.group<TransformComponent>(entt::get<SpriteRendererComponent>);
		renderQueue.reserve(group.size());

		for (auto entity : group)
		{
			auto [transform, sprite] = group.get<TransformComponent, SpriteRendererComponent>(entity);
			renderQueue.push_back({ GetWorldSpaceTransformMatrix({ entity, this }), &sprite, (int)entity });
		}

		std::ranges::sort(renderQueue, {}, [](const SpriteRenderData& d) {
			return d.Sprite->SortingOrder;
		});

		for (const auto& data : renderQueue)
		{
			const auto& sprite = *data.Sprite;

			if (sprite.SubTexture)
			{
				Renderer2D::DrawSprite(data.Transform,
					sprite.SubTexture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY, data.EntityID);
			}
			else if (sprite.TextureHandle != 0)
			{
				Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(sprite.TextureHandle);
				if (texture)
					Renderer2D::DrawSprite(data.Transform,
						texture, sprite.TilingFactor, sprite.Color, sprite.FlipX, sprite.FlipY, data.EntityID);
				else
					Renderer2D::DrawQuad(data.Transform, sprite.Color, data.EntityID);
			}
			else
			{
				Renderer2D::DrawQuad(data.Transform, sprite.Color, data.EntityID);
			}
		}

		Renderer2D::EndScene();

		RenderCommand::SetDepthTest(true);
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

	Entity Scene::FindEntityByName(const std::string& name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const auto& tag = view.get<TagComponent>(entity);
			if (tag.Tag == name)
				return Entity{ entity, this };
		}
		return Entity{};
	}

	Entity Scene::FindEntityByUUID(UUID uuid)
	{
		if (auto it = m_EntityIDMap.find(uuid); it != m_EntityIDMap.end())
			return Entity{ it->second, this };
		return Entity{};
	}

	// Lua Scripting
	void Scene::InitializeLuaScripts()
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto e : view)
		{
			auto& lsc = view.get<LuaScriptComponent>(e);
			if (!lsc.ScriptPath.empty() && !lsc.IsLoaded)
			{
				lsc.ScriptInstance = LuaScriptEngine::CreateScriptInstance(lsc.ScriptPath);
				if (lsc.ScriptInstance.valid())
				{
					lsc.IsLoaded = true;

					// 注入 Entity 对象
					lsc.ScriptInstance["entity"] = Entity{ e, this };

					// 缓存函数
					lsc.OnCreateFunc = lsc.ScriptInstance["OnCreate"];
					lsc.OnUpdateFunc = lsc.ScriptInstance["OnUpdate"];
					lsc.OnDestroyFunc = lsc.ScriptInstance["OnDestroy"];
					lsc.OnCollisionEnterFunc = lsc.ScriptInstance["OnCollisionEnter"];
					lsc.OnCollisionExitFunc = lsc.ScriptInstance["OnCollisionExit"];

					lsc.OnTriggerEnterFunc = lsc.ScriptInstance["OnTriggerEnter"];
					lsc.OnTriggerExitFunc = lsc.ScriptInstance["OnTriggerExit"];

					// 调用 OnCreate
					if (lsc.OnCreateFunc.valid())
						lsc.OnCreateFunc(lsc.ScriptInstance);
				}
				else
				{
					YUICY_CORE_ERROR("[Scene] Failed to load Lua script: {}", lsc.ScriptPath);
				}
			}
		}
	}

	void Scene::UpdateLuaScripts(Timestep ts)
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto e : view)
		{
			auto& lsc = view.get<LuaScriptComponent>(e);
			
			// 运行时初始化：处理新添加的脚本组件
			if (!lsc.ScriptPath.empty() && !lsc.IsLoaded)
			{
				lsc.ScriptInstance = LuaScriptEngine::CreateScriptInstance(lsc.ScriptPath);
				if (lsc.ScriptInstance.valid())
				{
					lsc.IsLoaded = true;
					lsc.ScriptInstance["entity"] = Entity{ e, this };
					
					lsc.OnCreateFunc = lsc.ScriptInstance["OnCreate"];
					lsc.OnUpdateFunc = lsc.ScriptInstance["OnUpdate"];
					lsc.OnDestroyFunc = lsc.ScriptInstance["OnDestroy"];
					lsc.OnCollisionEnterFunc = lsc.ScriptInstance["OnCollisionEnter"];
					lsc.OnCollisionExitFunc = lsc.ScriptInstance["OnCollisionExit"];
					lsc.OnTriggerEnterFunc = lsc.ScriptInstance["OnTriggerEnter"];
					lsc.OnTriggerExitFunc = lsc.ScriptInstance["OnTriggerExit"];
					
					if (lsc.OnCreateFunc.valid())
						lsc.OnCreateFunc(lsc.ScriptInstance);
				}
				else
				{
					YUICY_CORE_ERROR("[Scene] Failed to load runtime Lua script: {}", lsc.ScriptPath);
				}
			}
			
			// 调用 OnUpdate
			if (lsc.IsLoaded && lsc.OnUpdateFunc.valid())
			{
				try {
					auto result = lsc.OnUpdateFunc(lsc.ScriptInstance, (float)ts);
					if (!result.valid()) {
						sol::error err = result;
						YUICY_CORE_ERROR("[Lua Error] OnUpdate: {}", err.what());
					}
				}
				catch (const std::exception& e) {
					YUICY_CORE_ERROR("[Lua Error] OnUpdate Exception: {}", e.what());
				}
			}
		}
	}

	void Scene::DestroyLuaScripts()
	{
		auto view = m_Registry.view<LuaScriptComponent>();
		for (auto e : view)
		{
			auto& lsc = view.get<LuaScriptComponent>(e);
			if (lsc.IsLoaded && lsc.OnDestroyFunc.valid())
			{
				lsc.OnDestroyFunc(lsc.ScriptInstance);
			}
			lsc.IsLoaded = false;
		}
	}

	void Scene::ProcessLuaCollisionCallbacks()
	{
		if (!m_ContactListener) return;

		// Begin Contact
		for (const auto& contact : m_ContactListener->GetBeginContacts())
		{
			Entity entityA = { (entt::entity)(uintptr_t)contact.EntityA, this };
			Entity entityB = { (entt::entity)(uintptr_t)contact.EntityB, this };

			if (entityA.HasComponent<LuaScriptComponent>())
			{
				auto& lsc = entityA.GetComponent<LuaScriptComponent>();

				if (contact.IsSensorA || contact.IsSensorB)
				{
					if (lsc.IsLoaded && lsc.OnTriggerEnterFunc.valid())
						lsc.OnTriggerEnterFunc(lsc.ScriptInstance, entityB);
				}
				else
				{
					if (lsc.IsLoaded && lsc.OnCollisionEnterFunc.valid())
						lsc.OnCollisionEnterFunc(lsc.ScriptInstance, entityB);
				}
			}
			if (entityB.HasComponent<LuaScriptComponent>())
			{
				auto& lsc = entityB.GetComponent<LuaScriptComponent>();
				if (contact.IsSensorA || contact.IsSensorB)
				{
					if (lsc.IsLoaded && lsc.OnTriggerEnterFunc.valid())
						lsc.OnTriggerEnterFunc(lsc.ScriptInstance, entityA);
				}
				else
				{
					if (lsc.IsLoaded && lsc.OnCollisionEnterFunc.valid())
						lsc.OnCollisionEnterFunc(lsc.ScriptInstance, entityA);
				}
			}
		}

		// End Contact
		for (const auto& contact : m_ContactListener->GetEndContacts())
		{
			Entity entityA = { (entt::entity)(uintptr_t)contact.EntityA, this };
			Entity entityB = { (entt::entity)(uintptr_t)contact.EntityB, this };

			if (entityA.HasComponent<LuaScriptComponent>())
			{
				auto& lsc = entityA.GetComponent<LuaScriptComponent>();
				if (contact.IsSensorA || contact.IsSensorB)
				{
					if (lsc.IsLoaded && lsc.OnTriggerExitFunc.valid())
						lsc.OnTriggerExitFunc(lsc.ScriptInstance, entityB);
				}
				else
				{
					if (lsc.IsLoaded && lsc.OnCollisionExitFunc.valid())
						lsc.OnCollisionExitFunc(lsc.ScriptInstance, entityB);
				}
			}
			if (entityB.HasComponent<LuaScriptComponent>())
			{
				auto& lsc = entityB.GetComponent<LuaScriptComponent>();
				if (contact.IsSensorA || contact.IsSensorB)
				{
					if (lsc.IsLoaded && lsc.OnTriggerExitFunc.valid())
						lsc.OnTriggerExitFunc(lsc.ScriptInstance, entityA);
				}
				else
				{
					if (lsc.IsLoaded && lsc.OnCollisionExitFunc.valid())
						lsc.OnCollisionExitFunc(lsc.ScriptInstance, entityA);
				}
			}
		}
	}
}
