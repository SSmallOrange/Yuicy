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
		void SetupTileMap();
		void RegisterParsers();

		bool OnWindowResize(Yuicy::WindowResizeEvent& e);
		bool OnKeyPressed(Yuicy::KeyPressedEvent& e);
		bool OnMouseScrolled(Yuicy::MouseScrolledEvent& e);

	private:
		Yuicy::Ref<Yuicy::Scene> m_scene;
		Yuicy::Ref<Yuicy::TileMap> m_tileMap;
		Yuicy::Entity m_cameraEntity;

		glm::vec2 m_viewportSize = { 1280.0f, 720.0f };
		glm::vec4 m_clearColor = { 0.1f, 0.1f, 0.15f, 1.0f };

		float m_zoomLevel = 10.0f;      // 初始缩放级别（正交大小）
		float m_minZoom = 1.0f;         // 最小缩放
		float m_maxZoom = 50.0f;        // 最大缩放
		float m_zoomSpeed = 1.5f;       // 缩放速度
	};

}