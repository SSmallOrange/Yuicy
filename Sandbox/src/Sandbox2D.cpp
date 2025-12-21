#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Platform/OpenGL/OpenGLShader.h"
#include "Yuicy/Debug/Instrumentor.h"

// 引入脚本
#include "PlayerController.h"
#include "CameraController.h"

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

	// ==================== 创建玩家 ====================
	m_Player = m_ActiveScene->CreateEntity("Player");
	auto& playerTransform = m_Player.GetComponent<Yuicy::TransformComponent>();
	playerTransform.Translation = { 0.0f, 2.0f, 0.0f };
	playerTransform.Scale = { 0.8f, 0.8f, 1.0f };

	// 玩家精灵
	auto& playerSprite = m_Player.AddComponent<Yuicy::SpriteRendererComponent>();
	playerSprite.Color = { 0.2f, 0.8f, 0.3f, 1.0f };

	// 玩家物理
	auto& playerRb = m_Player.AddComponent<Yuicy::Rigidbody2DComponent>();
	playerRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	playerRb.FixedRotation = false;

	auto& playerCollider = m_Player.AddComponent<Yuicy::BoxCollider2DComponent>();
	playerCollider.Density = 1.0f;
	playerCollider.Friction = 0.3f;

	// 绑定玩家控制器脚本
	m_Player.AddComponent<Yuicy::NativeScriptComponent>().Bind<PlayerController>();

	// ==================== 创建相机 ====================
	m_CameraEntity = m_ActiveScene->CreateEntity("Camera");
	m_CameraEntity.AddComponent<Yuicy::CameraComponent>();
	auto& cameraTransform = m_CameraEntity.GetComponent<Yuicy::TransformComponent>();
	cameraTransform.Translation = { 0.0f, 2.0f, 0.0f };

	// 绑定相机控制器脚本
	auto& cameraScript = m_CameraEntity.AddComponent<Yuicy::NativeScriptComponent>();
	cameraScript.Bind<CameraController>();

	// ==================== 创建地面 ====================
	m_GroundEntity = m_ActiveScene->CreateEntity("Ground");
	auto& groundTransform = m_GroundEntity.GetComponent<Yuicy::TransformComponent>();
	groundTransform.Translation = { 0.0f, -2.0f, 0.0f };
	groundTransform.Scale = { 20.0f, 0.5f, 1.0f };

	auto& groundSprite = m_GroundEntity.AddComponent<Yuicy::SpriteRendererComponent>();
	groundSprite.Texture = m_CheckerboardTexture;
	groundSprite.TilingFactor = 10.0f;
	groundSprite.Color = { 0.4f, 0.4f, 0.4f, 1.0f };

	auto& groundRb = m_GroundEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
	groundRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	m_GroundEntity.AddComponent<Yuicy::BoxCollider2DComponent>();

	// ==================== 创建平台 ====================
	auto platform1 = m_ActiveScene->CreateEntity("Platform");
	auto& p1Transform = platform1.GetComponent<Yuicy::TransformComponent>();
	p1Transform.Translation = { 3.0f, 0.0f, 0.0f };
	p1Transform.Scale = { 3.0f, 0.3f, 1.0f };

	platform1.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.5f, 0.3f, 0.2f, 1.0f });
	platform1.AddComponent<Yuicy::Rigidbody2DComponent>().Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	platform1.AddComponent<Yuicy::BoxCollider2DComponent>();

	auto platform2 = m_ActiveScene->CreateEntity("Platform");
	auto& p2Transform = platform2.GetComponent<Yuicy::TransformComponent>();
	p2Transform.Translation = { -3.0f, 1.5f, 0.0f };
	p2Transform.Scale = { 2.5f, 0.3f, 1.0f };

	platform2.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.5f, 0.3f, 0.2f, 1.0f });
	platform2.AddComponent<Yuicy::Rigidbody2DComponent>().Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	platform2.AddComponent<Yuicy::BoxCollider2DComponent>();

	// ==================== 创建触发器（示例：金币）====================
	auto coin = m_ActiveScene->CreateEntity("Coin");
	auto& coinTransform = coin.GetComponent<Yuicy::TransformComponent>();
	coinTransform.Translation = { 3.0f, 1.5f, 0.0f };
	coinTransform.Scale = { 0.5f, 0.5f, 1.0f };

	auto& coinSprite = coin.AddComponent<Yuicy::SpriteRendererComponent>();
	coinSprite.Color = { 1.0f, 0.8f, 0.0f, 1.0f };  // 金色

	auto& coinRb = coin.AddComponent<Yuicy::Rigidbody2DComponent>();
	coinRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

	auto& coinCollider = coin.AddComponent<Yuicy::BoxCollider2DComponent>();
	coinCollider.IsTrigger = true;  // 设置为触发器

	// 初始化
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

	// 启动场景（会初始化物理和脚本）
	m_ActiveScene->OnRuntimeStart();

	// 设置相机跟随目标（在脚本初始化后）
	if (m_CameraEntity.HasComponent<Yuicy::NativeScriptComponent>())
	{
		auto& nsc = m_CameraEntity.GetComponent<Yuicy::NativeScriptComponent>();
		if (nsc.Instance)
		{
			auto* cameraController = static_cast<CameraController*>(nsc.Instance);
			cameraController->Target = m_Player;
		}
	}
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
	ImGui::Begin("Script Demo");

	auto stats = Yuicy::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);

	ImGui::Separator();
	ImGui::Text("Controls:");
	ImGui::Text("A/D or Left/Right: Move");
	ImGui::Text("Space/W/Up: Jump");

	if (m_Player)
	{
		ImGui::Separator();
		auto& transform = m_Player.GetComponent<Yuicy::TransformComponent>();
		ImGui::Text("Player Position: (%.2f, %.2f)",
			transform.Translation.x, transform.Translation.y);
	}

	ImGui::Separator();
	ImGui::Text("Press 'R' to restart");

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

	dispatcher.Dispatch<Yuicy::KeyPressedEvent>([this](Yuicy::KeyPressedEvent& e) {
		if (e.GetKeyCode() == Yuicy::Key::R)
		{
			m_ActiveScene->OnRuntimeStop();

			// 重置玩家位置
			if (m_Player)
			{
				auto& transform = m_Player.GetComponent<Yuicy::TransformComponent>();
				transform.Translation = { 0.0f, 2.0f, 0.0f };
			}

			m_ActiveScene->OnRuntimeStart();

			// 重新设置相机跟随
			if (m_CameraEntity.HasComponent<Yuicy::NativeScriptComponent>())
			{
				auto& nsc = m_CameraEntity.GetComponent<Yuicy::NativeScriptComponent>();
				if (nsc.Instance)
				{
					auto* cameraController = static_cast<CameraController*>(nsc.Instance);
					cameraController->Target = m_Player;
				}
			}
		}
		return false;
	});
}