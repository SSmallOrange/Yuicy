#include <Yuicy.h>

#include <imgui/imgui.h>

#include "GameLayer.h"

namespace TinyDungeon {

	GameLayer::GameLayer()
		: Layer("TinyDungeon")
		, m_cameraController(16.0f / 9.0f, false)
	{
	}

	void GameLayer::OnAttach()
	{
		SetupCamera();
		SetupScene();
	}

	void GameLayer::OnDetach()
	{
		m_scene.reset();
	}

	void GameLayer::OnUpdate(Yuicy::Timestep ts)
	{
		m_cameraController.OnUpdate(ts);

		Yuicy::RenderCommand::SetClearColor(m_clearColor);
		Yuicy::RenderCommand::Clear();

		Yuicy::Renderer2D::BeginScene(m_cameraController.GetCamera());

		Yuicy::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, { 0.8f, 0.2f, 0.3f, 1.0f });
		Yuicy::Renderer2D::DrawQuad({ 1.5f, 0.0f }, { 1.0f, 1.0f }, { 0.2f, 0.8f, 0.3f, 1.0f });
		Yuicy::Renderer2D::DrawQuad({ -1.5f, 0.0f }, { 1.0f, 1.0f }, { 0.2f, 0.3f, 0.8f, 1.0f });

		Yuicy::Renderer2D::EndScene();
	}

	void GameLayer::OnImGuiRender()
	{
		ImGui::Begin("Debug");
		ImGui::ColorEdit4("Clear Color", &m_clearColor.x);
		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
		ImGui::End();
	}

	void GameLayer::OnEvent(Yuicy::Event& e)
	{
		m_cameraController.OnEvent(e);
	}

	void GameLayer::SetupCamera()
	{
		m_cameraController.SetZoomLevel(5.0f);
	}

	void GameLayer::SetupScene()
	{
		m_scene = Yuicy::CreateScope<Yuicy::Scene>();
	}

}