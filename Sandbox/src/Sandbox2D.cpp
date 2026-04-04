#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Platform/OpenGL/OpenGLShader.h"
#include "Yuicy/Debug/Instrumentor.h"

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, true), m_ParticleSystem(2000)
{
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
}

void Sandbox2D::OnAttach()
{
	m_CheckerboardTexture = Yuicy::Texture2D::Create("assets/textures/Checkerboard.png");

	m_ActiveScene = Yuicy::CreateRef<Yuicy::Scene>();

	// ==================== 创建基本测试实体 ====================

	// 创建相机
	auto cameraEntity = m_ActiveScene->CreateEntity("Camera");
	cameraEntity.AddComponent<Yuicy::CameraComponent>();

	// 创建地面
	auto groundEntity = m_ActiveScene->CreateEntity("Ground");
	auto& groundTransform = groundEntity.GetComponent<Yuicy::TransformComponent>();
	groundTransform.Translation = { 0.0f, -2.0f, 0.0f };
	groundTransform.Scale = { 20.0f, 0.5f, 1.0f };

	auto& groundSprite = groundEntity.AddComponent<Yuicy::SpriteRendererComponent>();
	groundSprite.Texture = m_CheckerboardTexture;
	groundSprite.TilingFactor = 10.0f;
	groundSprite.Color = { 0.4f, 0.4f, 0.4f, 1.0f };

	auto& groundRb = groundEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
	groundRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	groundEntity.AddComponent<Yuicy::BoxCollider2DComponent>();

	// 创建动态方块
	auto boxEntity = m_ActiveScene->CreateEntity("Box");
	auto& boxTransform = boxEntity.GetComponent<Yuicy::TransformComponent>();
	boxTransform.Translation = { 0.0f, 3.0f, 0.0f };
	boxTransform.Scale = { 1.0f, 1.0f, 1.0f };

	boxEntity.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.2f, 0.6f, 0.8f, 1.0f });
	auto& boxRb = boxEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
	boxRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	boxEntity.AddComponent<Yuicy::BoxCollider2DComponent>();

	// 初始化
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

	// 启动场景
	m_ActiveScene->OnRuntimeStart();
}

void Sandbox2D::OnDetach()
{
	m_ActiveScene->OnRuntimeStop();
}

void Sandbox2D::OnUpdate(Yuicy::Timestep ts)
{
	YUICY_PROFILE_FUNCTION();

	Yuicy::Renderer2D::ResetStats();

	Yuicy::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1 });
	Yuicy::RenderCommand::Clear();

	m_ActiveScene->OnUpdateRuntime(ts);
}

void Sandbox2D::OnImGuiRender()
{
	YUICY_PROFILE_FUNCTION();
	ImGui::Begin("Sandbox2D");

	auto stats = Yuicy::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);

	ImGui::End();
}

void Sandbox2D::OnEvent(Yuicy::Event& e)
{
	Yuicy::EventDispatcher dispatcher(e);

	dispatcher.Dispatch<Yuicy::WindowResizeEvent>([this](Yuicy::WindowResizeEvent& e) {
		if (e.GetHeight() == 0.0f)
			return false;
		m_ViewportSize = { (float)e.GetWidth(), (float)e.GetHeight() };
		m_ActiveScene->OnViewportResize(e.GetWidth(), e.GetHeight());
		return false;
	});
}