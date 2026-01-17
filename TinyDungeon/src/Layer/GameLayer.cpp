#include <Yuicy.h>
#include <imgui/imgui.h>

#include "GameLayer.h"
#include "../TileMap/DungeonMapParser.h"
#include "../TileMap/DungeonMapBuilder.h"

namespace TinyDungeon {

	GameLayer::GameLayer()
		: Layer("TinyDungeon")
	{
	}

	void GameLayer::OnAttach()
	{
		m_viewportSize = {
			Yuicy::Application::Get().GetWindow().GetWidth(),
			Yuicy::Application::Get().GetWindow().GetHeight()
		};

		RegisterParsers();
		SetupScene();
		SetupCamera();
		SetupTileMap();
		SetupPlayer();

		m_scene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_scene->OnRuntimeStart();

		// Framebuffer for post-processing
		Yuicy::FramebufferSpecification fbSpec;
		fbSpec.width = (uint32_t)m_viewportSize.x;
		fbSpec.height = (uint32_t)m_viewportSize.y;
		fbSpec.attachments = {
			Yuicy::FramebufferTextureFormat::RGBA8,
			Yuicy::FramebufferTextureFormat::DEPTH24STENCIL8
		};
		m_framebuffer = Yuicy::Framebuffer::Create(fbSpec);

		// Post-processing
		m_postProcessing.Init();

 		Yuicy::PostProcessConfig nightConfig;
 		nightConfig.brightness = 0.6f;
 		nightConfig.saturation = 0.7f;
 		nightConfig.ambientTint = { 0.6f, 0.65f, 0.85f, 1.0f };
 		nightConfig.vignetteEnabled = true;
 		nightConfig.vignetteIntensity = 0.25f;  // Reduced from 0.5
 		nightConfig.vignetteRadius = 0.9f;      // Larger radius = less dark area
 		m_postProcessing.FadeTo(nightConfig, 1.5f);

		m_postProcessing.SetRaindropsEnabled(true);
		m_postProcessing.SetRaindropsIntensity(1.0f);


		Yuicy::WeatherConfig rainConfig = Yuicy::WeatherPresets::Get(Yuicy::WeatherType::Rain);
		
		rainConfig.particles.spawnRate = 150.0f;                        // 生成速率
		rainConfig.particles.sizeMin = 0.06f;                           // 尺寸
		rainConfig.particles.sizeMax = 0.12f;
		rainConfig.particles.colorStart = { 0.8f, 0.9f, 1.0f, 1.0f };
		rainConfig.particles.colorEnd = { 0.7f, 0.85f, 1.0f, 0.3f };

		// 雨滴溅射效果
		rainConfig.particles.splashConfig.sizeMin = 0.05f;
		rainConfig.particles.splashConfig.sizeMax = 0.1f;
		rainConfig.particles.splashConfig.particleCount = 8;
		rainConfig.particles.splashConfig.colorStart = { 0.8f, 0.9f, 1.0f, 0.9f };

		rainConfig.particles.enablePhysics = true;
		rainConfig.particles.physicsRatio = 0.3f;

		m_weatherSystem.SetWeather(rainConfig);
		m_weatherSystem.SetPhysicsWorld(m_scene->GetPhysicsWorld());

		// 2D Lighting system
		m_lighting = Yuicy::CreateRef<Yuicy::Lighting2D>();
		m_lighting->Init((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_lighting->SetEnabled(true);

		// Ambient lighting config (dark atmosphere)
		Yuicy::LightingConfig lightConfig;
		lightConfig.enabled = true;
		lightConfig.ambientColor = { 0.2f, 0.2f, 0.25f };  // Very dark ambient
		lightConfig.ambientIntensity = 0.7f;
		m_lighting->SetConfig(lightConfig);

		// Flashlight (spot light)
		Yuicy::Light2D flashlight;
		flashlight.type = Yuicy::Light2DType::Spot;
		flashlight.color = { 1.0f, 0.95f, 0.85f };  // Warm white
		flashlight.intensity = 1.0f;
		flashlight.radius = 10.0f;  // 10 world units range
		flashlight.falloff = 1.5f;
		flashlight.innerAngle = 0.2f;  // ~23 degrees inner
		flashlight.outerAngle = 0.4f;  // ~45 degrees outer
		flashlight.direction = 0.0f;   // Will be updated by mouse
		flashlight.castShadows = false;  // Start without shadows for performance
		m_flashlightId = m_lighting->AddLight(flashlight);

		// Yellow point light (e.g., torch/lamp)
		Yuicy::Light2D pointLight;
		pointLight.type = Yuicy::Light2DType::Point;
		pointLight.position = { 5.0f, 12.0f };
		pointLight.color = { 1.0f, 0.8f, 0.3f };
		pointLight.intensity = 1.2f;
		pointLight.radius = 1.0f;
		pointLight.falloff = 3.0f;
		m_lighting->AddLight(pointLight);

		// Window Overlay
		m_windowOverlay = Yuicy::WindowOverlay::Create();
		Yuicy::WindowOverlayConfig overlayConfig;
		overlayConfig.closeButtonTexture = "assets/textures/closeWindow.png";
		overlayConfig.minimizeButtonTexture = "assets/textures/minimize.png";
		overlayConfig.showMaximizeButton = false;
		m_windowOverlay->SetConfig(overlayConfig);
		m_windowOverlay->Init();
	}

	void GameLayer::OnDetach()
	{
		m_scene->OnRuntimeStop();
	}

	void GameLayer::OnUpdate(Yuicy::Timestep ts)
	{
		m_weatherSystem.OnUpdate(ts);
		m_postProcessing.OnUpdate(ts);
		m_lighting->OnUpdate(ts);

		// Update flashlight position and direction
 		if (m_playerEntity && m_flashlightId != 0)
 		{
 			auto& playerTransform = m_playerEntity.GetComponent<Yuicy::TransformComponent>();
 			glm::vec2 playerPos = { playerTransform.Translation.x, playerTransform.Translation.y };
 
 			auto [mouseX, mouseY] = Yuicy::Input::GetMousePosition();
 			
 			if (m_cameraEntity)
 			{
 				auto& camTransform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
 				auto& camComp = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
 				float orthoSize = camComp.Camera.GetOrthographicSize();
 				float aspectRatio = m_viewportSize.x / m_viewportSize.y;
 
 				// Screen to NDC [-1, 1]
 				float ndcX = (mouseX / m_viewportSize.x) * 2.0f - 1.0f;
 				float ndcY = 1.0f - (mouseY / m_viewportSize.y) * 2.0f;  // Flip Y
 
 				// NDC to world
 				float halfHeight = orthoSize / 2.0f;
 				float halfWidth = halfHeight * aspectRatio;
 				float worldX = camTransform.Translation.x + ndcX * halfWidth;
 				float worldY = camTransform.Translation.y + ndcY * halfHeight;
 
 				// Calculate direction from player to mouse
 				glm::vec2 mouseWorld = { worldX, worldY };
 				glm::vec2 dir = mouseWorld - playerPos;
 				float angle = std::atan2(dir.y, dir.x);
 
 				// Update flashlight
 				if (auto* light = m_lighting->GetLight(m_flashlightId))
 				{
 					light->position = playerPos;
 					light->direction = angle;
 				}
 			}
 		}

		m_framebuffer->Bind();

		Yuicy::Renderer2D::ResetStats();
		Yuicy::RenderCommand::SetClearColor(m_clearColor);
		Yuicy::RenderCommand::Clear();

		// 场景渲染
		m_scene->OnUpdateRuntime(ts);

		// 天气渲染
		if (m_cameraEntity && m_weatherSystem.IsActive())
		{
			auto& cameraComp = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
			auto& camTransform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();

			float orthoSize = cameraComp.Camera.GetOrthographicSize();
			float aspectRatio = m_viewportSize.x / m_viewportSize.y;
			glm::vec2 viewportWorldSize = { orthoSize * aspectRatio, orthoSize };

			Yuicy::Renderer2D::BeginScene(cameraComp.Camera, camTransform.GetTransform());
			m_weatherSystem.OnRender(
				{ camTransform.Translation.x, camTransform.Translation.y },
				viewportWorldSize
			);
			Yuicy::Renderer2D::EndScene();
		}

		m_framebuffer->Unbind();

		// Render lighting
		if (m_cameraEntity && m_lighting->IsEnabled())
		{
			auto& cameraComp = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
			auto& camTransform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();

			float orthoSize = cameraComp.Camera.GetOrthographicSize();
			float aspectRatio = m_viewportSize.x / m_viewportSize.y;
			glm::vec2 viewportWorldSize = { orthoSize * aspectRatio, orthoSize };

			m_lighting->RenderLightMap(
				{ camTransform.Translation.x, camTransform.Translation.y },
				viewportWorldSize
			);
		}

		// Post-processing (with lighting)
		Yuicy::RenderCommand::SetClearColor({ 0, 0, 0, 1 });
		Yuicy::RenderCommand::Clear();

		auto& ppconfig = m_postProcessing.GetConfig();
		ppconfig.lightingEnabled = m_lighting->IsEnabled();
		ppconfig.lightMapTextureID = m_lighting->GetLightMapTextureID();

		m_postProcessing.Render(m_framebuffer);
	}

	void GameLayer::OnImGuiRender()
	{
		if (m_windowOverlay)
			m_windowOverlay->OnImGuiRender();

		ImGui::Begin("TinyDungeon Debug");

		auto stats = Yuicy::Renderer2D::GetStats();
		ImGui::Text("Draw Calls: %d | Quads: %d | FPS: %.1f", stats.DrawCalls, stats.QuadCount, ImGui::GetIO().Framerate);

		// 相机、玩家位置
		ImGui::Separator();
		if (m_playerEntity)
		{
			auto& pt = m_playerEntity.GetComponent<Yuicy::TransformComponent>();
			ImGui::Text("Player: (%.2f, %.2f)", pt.Translation.x, pt.Translation.y);
		}
		if (m_cameraEntity)
		{
			auto& ct = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
			ImGui::Text("Camera: (%.2f, %.2f) | Zoom: %.1f", ct.Translation.x, ct.Translation.y, m_zoomLevel);
		}

		// 雨滴颜色调整
		if (m_weatherSystem.IsActive())
		{
			ImGui::Separator();
			ImGui::Text("Rain Particles:");
			auto& weatherConfig = m_weatherSystem.getConfig();
			ImGui::SliderFloat("Particle Alpha Start", &weatherConfig.particles.colorStart.a, 0.0f, 1.0f);
			ImGui::SliderFloat("Particle Alpha End", &weatherConfig.particles.colorEnd.a, 0.0f, 1.0f);
			ImGui::ColorEdit3("Particle Color", &weatherConfig.particles.colorStart.r);
		}

		ImGui::End();
	}

	void GameLayer::OnEvent(Yuicy::Event& e)
	{
		Yuicy::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<Yuicy::WindowResizeEvent>(std::bind(&GameLayer::OnWindowResize, this, std::placeholders::_1));
		dispatcher.Dispatch<Yuicy::KeyPressedEvent>(std::bind(&GameLayer::OnKeyPressed, this, std::placeholders::_1));
	}

	void GameLayer::SetupScene()
	{
		m_scene = Yuicy::CreateRef<Yuicy::Scene>();
	}

	void GameLayer::SetupCamera()
	{
		m_cameraEntity = m_scene->CreateEntity("MainCamera");
		auto& camera = m_cameraEntity.AddComponent<Yuicy::CameraComponent>();
		camera.Primary = true;
		camera.Camera.SetOrthographicSize(m_zoomLevel);

		// Use Lua script for camera follow
		m_cameraEntity.AddComponent<Yuicy::LuaScriptComponent>("assets/scripts/camera_controller.lua");

		// Initial position
		auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
		float aspectRatio = m_viewportSize.x / m_viewportSize.y;
		float halfHeight = m_zoomLevel / 2.0f;
		float halfWidth = halfHeight * aspectRatio;
		transform.Translation = { halfWidth, halfHeight, 0.0f };
	}

	void GameLayer::SetupPlayer()
	{
		m_playerEntity = m_scene->CreateEntity("Player");

		auto& transform = m_playerEntity.GetComponent<Yuicy::TransformComponent>();
		transform.Translation = { 2.5f, 11.5f, 0.9f };

		auto& sprite = m_playerEntity.AddComponent<Yuicy::SpriteRendererComponent>();
		sprite.SortingOrder = 1000; 

		// 使用Lua脚本控制玩家
		m_playerEntity.AddComponent<Yuicy::LuaScriptComponent>("assets/scripts/player_controller.lua");

		// 玩家动画组件
		auto& playerAnimation = m_playerEntity.AddComponent<Yuicy::AnimationComponent>();

		// 动画纹理
		auto animTexture = Yuicy::Texture2D::Create("assets/textures/map/tilemap-characters_packed.png");

		// 待机动画
		Yuicy::AnimationClip idleClip("idle", 1.0f, true);
		idleClip.AddFramesFromSheet(animTexture, { 0, 2 }, 1, { 24, 24 });
		playerAnimation.AddClip(idleClip);

		// 移动动画
		Yuicy::AnimationClip jumpClipRight("walk_right", 0.1f, false);  // 不循环
		jumpClipRight.AddFramesFromSheet(animTexture, { 0, 2 }, 2, { 24, 24 });
		playerAnimation.AddClip(jumpClipRight);

		Yuicy::AnimationClip jumpClipLeft("walk_left", 0.1f, false);  // 不循环
		jumpClipLeft.AddFramesFromSheet(animTexture, { 0, 2 }, 2, { 24, 24 });
		playerAnimation.AddClip(jumpClipLeft);

		playerAnimation.Play("idle");

		// 物理组件
		auto& playerRb = m_playerEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
		playerRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
		playerRb.FixedRotation = true;

		auto& playerCollider = m_playerEntity.AddComponent<Yuicy::CircleCollider2DComponent>();
		playerCollider.Radius = 0.4f;
		playerCollider.Friction = 0.0f;
	}

	// 注册地图解析器	
	void GameLayer::RegisterParsers()
	{
		auto& loader = Yuicy::TileMapSystem::GetLoader();
		loader.RegisterParser(Yuicy::CreateRef<DungeonMapParser>());
	}

	void GameLayer::SetupTileMap()
	{
		auto builder = Yuicy::CreateRef<DungeonMapBuilder>();

		m_tileMap = Yuicy::TileMapSystem::LoadMap("assets/maps/SampleB.json", m_scene.get(), builder);

		if (!m_tileMap)
		{
			YUICY_CORE_WARN("GameLayer: Failed to load SampleB, using test entities");

			// 测试实体
			auto testQuad = m_scene->CreateEntity("TestQuad");
			testQuad.GetComponent<Yuicy::TransformComponent>().Translation = { 0.0f, 0.0f, 0.0f };
			testQuad.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.8f, 0.2f, 0.3f, 1.0f });
		}
	}

	bool GameLayer::OnWindowResize(Yuicy::WindowResizeEvent& e)
	{
		if (e.GetHeight() == 0)
			return false;

		m_viewportSize = { (float)e.GetWidth(), (float)e.GetHeight() };
		m_scene->OnViewportResize(e.GetWidth(), e.GetHeight());
		m_framebuffer->Resize(e.GetWidth(), e.GetHeight());
		m_lighting->Resize(e.GetWidth(), e.GetHeight());
		return false;
	}

	bool GameLayer::OnKeyPressed(Yuicy::KeyPressedEvent& e)
	{
		// 按 R 键重置相机位置和缩放
		if (e.GetKeyCode() == Yuicy::Key::R)
		{
			if (m_cameraEntity)
			{
				auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
				transform.Translation = { 15.0f, 15.0f, 0.0f };
				m_zoomLevel = 10.0f;

				auto& camera = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
				camera.Camera.SetOrthographicSize(m_zoomLevel);
			}
		}
		return false;
	}
}
