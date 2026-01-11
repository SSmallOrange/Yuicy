#pragma once

#include <Yuicy.h>

namespace TinyDungeon {

	class GameLayer : public Yuicy::Layer
	{
	public:
		GameLayer();
		~GameLayer() override = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(Yuicy::Timestep ts) override;
		void OnImGuiRender() override;
		void OnEvent(Yuicy::Event& e) override;

	private:
		void SetupScene();
		void SetupCamera();
		void SetupPlayer();
		void SetupTileMap();
		void RegisterParsers();

		bool OnWindowResize(Yuicy::WindowResizeEvent& e);
		bool OnKeyPressed(Yuicy::KeyPressedEvent& e);
		bool OnMouseScrolled(Yuicy::MouseScrolledEvent& e);

	private:
		Yuicy::Ref<Yuicy::Scene> m_scene;
		Yuicy::Ref<Yuicy::TileMap> m_tileMap;
		Yuicy::Entity m_cameraEntity;
		Yuicy::Entity m_playerEntity;

		// Map size in world units (50x30 tiles)
		const float m_mapWidth = 50.0f;
		const float m_mapHeight = 30.0f;

		glm::vec2 m_viewportSize = { 960.0f, 576.0f };
		glm::vec4 m_clearColor = { 0.1f, 0.1f, 0.15f, 1.0f };

		float m_zoomLevel = 8.0f;       // orthographic size (smaller = closer view, allows camera follow)
		float m_minZoom = 5.0f;         // min zoom
		float m_maxZoom = 15.0f;        // max zoom (15 shows entire map)
		float m_zoomSpeed = 1.0f;       // zoom speed
		float m_playerSpeed = 8.0f;     // player movement speed
	};

}