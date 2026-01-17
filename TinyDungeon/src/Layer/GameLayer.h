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

	private:
		Yuicy::Ref<Yuicy::Scene> m_scene;
		Yuicy::Ref<Yuicy::TileMap> m_tileMap;
		Yuicy::Entity m_cameraEntity;
		Yuicy::Entity m_playerEntity;

		const float m_mapWidth = 50.0f;
		const float m_mapHeight = 30.0f;

		glm::vec2 m_viewportSize = { 960.0f, 576.0f };
		glm::vec4 m_clearColor = { 0.1f, 0.1f, 0.15f, 1.0f };

		float m_zoomLevel = 15.0f;

		// Effects
		Yuicy::WeatherSystem m_weatherSystem{ 3000 };
		Yuicy::PostProcessing m_postProcessing;
		Yuicy::Ref<Yuicy::Framebuffer> m_framebuffer;

		// 2D Lighting
		Yuicy::Ref<Yuicy::Lighting2D> m_lighting;
		uint32_t m_flashlightId = 0;

		// Window Overlay (engine provided)
		Yuicy::Scope<Yuicy::WindowOverlay> m_windowOverlay;
	};

}