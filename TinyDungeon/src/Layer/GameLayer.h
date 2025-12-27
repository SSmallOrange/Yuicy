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
		void SetupCamera();
		void SetupScene();

	private:
		Yuicy::OrthographicCameraController m_cameraController;
		Yuicy::Scope<Yuicy::Scene> m_scene;

		glm::vec4 m_clearColor = { 0.1f, 0.1f, 0.15f, 1.0f };
	};

}