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

		m_scene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_scene->OnRuntimeStart();
	}

	void GameLayer::OnDetach()
	{
		m_scene->OnRuntimeStop();
	}

	void GameLayer::OnUpdate(Yuicy::Timestep ts)
	{
		// 相机移动
		if (m_cameraEntity)
		{
			auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
			float cameraSpeed = m_zoomLevel * ts;

			if (Yuicy::Input::IsKeyPressed(Yuicy::Key::A) || Yuicy::Input::IsKeyPressed(Yuicy::Key::Left))
				transform.Translation.x -= cameraSpeed;
			if (Yuicy::Input::IsKeyPressed(Yuicy::Key::D) || Yuicy::Input::IsKeyPressed(Yuicy::Key::Right))
				transform.Translation.x += cameraSpeed;
			if (Yuicy::Input::IsKeyPressed(Yuicy::Key::W) || Yuicy::Input::IsKeyPressed(Yuicy::Key::Up))
				transform.Translation.y += cameraSpeed;
			if (Yuicy::Input::IsKeyPressed(Yuicy::Key::S) || Yuicy::Input::IsKeyPressed(Yuicy::Key::Down))
				transform.Translation.y -= cameraSpeed;
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

		// 相机信息
		if (m_cameraEntity)
		{
			ImGui::Separator();
			ImGui::Text("Camera:");
			auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
			ImGui::Text("Position: (%.2f, %.2f)", transform.Translation.x, transform.Translation.y);
			ImGui::Text("Zoom Level: %.2f", m_zoomLevel);

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

		auto& transform = m_cameraEntity.GetComponent<Yuicy::TransformComponent>();
		transform.Translation = { 15.0f, 15.0f, 0.0f };
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

		m_tileMap = Yuicy::TileMapSystem::LoadMap("assets/maps/TileMap.json", m_scene.get(), builder);

		if (!m_tileMap)
		{
			YUICY_CORE_WARN("GameLayer: Failed to load tilemap, using test entities");

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