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
	}

	void GameLayer::OnDetach()
	{
		m_scene->OnRuntimeStop();
	}

	void GameLayer::OnUpdate(Yuicy::Timestep ts)
	{
		// Player movement now handled by Lua script (player_controller.lua)
		// Camera follows player with boundary clamping
		if (m_cameraEntity && m_playerEntity)
		{
			auto& cameraTransform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
			auto& playerTransform = m_playerEntity.GetComponent<Yuicy::TransformComponent>();

			// Camera target is player position (center on player)
			float targetX = playerTransform.Translation.x;
			float targetY = playerTransform.Translation.y;

			// Calculate visible area half-size
			// OrthographicSize is the FULL height, so half-height = zoomLevel / 2
			float aspectRatio = m_viewportSize.x / m_viewportSize.y;
			float halfHeight = m_zoomLevel / 2.0f;
			float halfWidth = halfHeight * aspectRatio;

			// Clamp camera center so edges don't go outside the map [0, mapWidth] x [0, mapHeight]
			float minCamX = halfWidth;
			float maxCamX = m_mapWidth - halfWidth;
			float minCamY = halfHeight;
			float maxCamY = m_mapHeight - halfHeight;

			// Handle case where map is smaller than visible area (center the camera)
			if (minCamX > maxCamX) targetX = m_mapWidth / 2.0f;
			else targetX = std::clamp(targetX, minCamX, maxCamX);

			if (minCamY > maxCamY) targetY = m_mapHeight / 2.0f;
			else targetY = std::clamp(targetY, minCamY, maxCamY);

			cameraTransform.Translation.x = targetX;
			cameraTransform.Translation.y = targetY;
		}

		Yuicy::Renderer2D::ResetStats();
		Yuicy::RenderCommand::SetClearColor(m_clearColor);
		Yuicy::RenderCommand::Clear();

		m_scene->OnUpdateRuntime(ts);
	}

	void GameLayer::OnImGuiRender()
	{
		ImGui::Begin("TinyDungeon Debug");

		auto stats = Yuicy::Renderer2D::GetStats();
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::Separator();
		ImGui::ColorEdit4("Clear Color", &m_clearColor.x);

		// Debug camera bounds
		float aspectRatio = m_viewportSize.x / m_viewportSize.y;
		float halfHeight = m_zoomLevel / 2.0f;  // OrthographicSize is full height
		float halfWidth = halfHeight * aspectRatio;
		ImGui::Separator();
		ImGui::Text("Viewport: %.0f x %.0f", m_viewportSize.x, m_viewportSize.y);
		ImGui::Text("Aspect: %.3f", aspectRatio);
		ImGui::Text("Half Size: %.2f x %.2f", halfWidth, halfHeight);
		ImGui::Text("Cam Bounds X: [%.2f, %.2f]", halfWidth, m_mapWidth - halfWidth);
		ImGui::Text("Cam Bounds Y: [%.2f, %.2f]", halfHeight, m_mapHeight - halfHeight);

		// Player info
		ImGui::Separator();
		ImGui::Text("Player Valid: %s", m_playerEntity ? "YES" : "NO");
		if (m_playerEntity)
		{
			auto& playerTransform = m_playerEntity.GetComponent<Yuicy::TransformComponent>();
			ImGui::Text("Player Pos: (%.2f, %.2f)", playerTransform.Translation.x, playerTransform.Translation.y);
		}

		// Camera info
		ImGui::Text("Camera Valid: %s", m_cameraEntity ? "YES" : "NO");
		if (m_cameraEntity)
		{
			auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
			ImGui::Text("Camera Pos: (%.2f, %.2f)", transform.Translation.x, transform.Translation.y);
			ImGui::Text("Zoom Level: %.2f", m_zoomLevel);
			
			// Show visible area
			ImGui::Text("Visible X: [%.2f, %.2f]", transform.Translation.x - halfWidth, transform.Translation.x + halfWidth);
			ImGui::Text("Visible Y: [%.2f, %.2f]", transform.Translation.y - halfHeight, transform.Translation.y + halfHeight);

			if (ImGui::SliderFloat("Zoom", &m_zoomLevel, m_minZoom, m_maxZoom))
			{
				auto& camera = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
				camera.Camera.SetOrthographicSize(m_zoomLevel);
			}
		}

		if (m_tileMap && m_tileMap->GetMapData())
		{
			ImGui::Separator();
			ImGui::Text("Map: %s", m_tileMap->GetMapData()->GetName().c_str());
			ImGui::Text("Entities: %zu", m_tileMap->GetEntities().size());
		}

		ImGui::End();
	}

	void GameLayer::OnEvent(Yuicy::Event& e)
	{
		Yuicy::EventDispatcher dispatcher(e);
		dispatcher.Dispatch<Yuicy::WindowResizeEvent>(std::bind(&GameLayer::OnWindowResize, this, std::placeholders::_1));
		dispatcher.Dispatch<Yuicy::KeyPressedEvent>(std::bind(&GameLayer::OnKeyPressed, this, std::placeholders::_1));
		dispatcher.Dispatch<Yuicy::MouseScrolledEvent>(std::bind(&GameLayer::OnMouseScrolled, this, std::placeholders::_1));
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

		// Initial position - will be immediately adjusted by camera follow in first OnUpdate
		// Set to minimum camera bounds (bottom-left corner of map view)
		auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
		float aspectRatio = m_viewportSize.x / m_viewportSize.y;
		float halfHeight = m_zoomLevel / 2.0f;  // OrthographicSize is full height
		float halfWidth = halfHeight * aspectRatio;
		transform.Translation = { halfWidth, halfHeight, 0.0f };
	}

	void GameLayer::SetupPlayer()
	{
		m_playerEntity = m_scene->CreateEntity("Player");

		// Spawn at bottom-left corner of the map (with 0.5 offset for center)
		auto& transform = m_playerEntity.GetComponent<Yuicy::TransformComponent>();
		transform.Translation = { 1.5f, 1.5f, 0.9f };

		// Yellow rectangle as player placeholder
		auto& sprite = m_playerEntity.AddComponent<Yuicy::SpriteRendererComponent>();
		sprite.Color = { 1.0f, 1.0f, 0.0f, 1.0f };  // Yellow
		sprite.SortingOrder = 1000;  // Render on top of tiles

		// 使用Lua脚本控制玩家
		m_playerEntity.AddComponent<Yuicy::LuaScriptComponent>("assets/scripts/player_controller.lua");
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

	bool GameLayer::OnMouseScrolled(Yuicy::MouseScrolledEvent& e)
	{
		if (m_cameraEntity)
		{
			m_zoomLevel -= e.GetYOffset() * m_zoomSpeed;
			m_zoomLevel = std::clamp(m_zoomLevel, m_minZoom, m_maxZoom);

			auto& camera = m_cameraEntity.GetComponent<Yuicy::CameraComponent>();
			camera.Camera.SetOrthographicSize(m_zoomLevel);
		}
		return false;
	}

}