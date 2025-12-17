#include "Sandbox2D.h"
#include "imgui/imgui.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Platform/OpenGL/OpenGLShader.h"
#include "Yuicy/Debug/Instrumentor.h"

Sandbox2D::Sandbox2D()
	: Layer("Sandbox2D"), m_CameraController(1280.0f / 720.0f, true), m_ParticleSystem(2000)
{
	m_ParticleProps.ColorBegin = { 1.0f, 0.5f, 0.2f, 1.0f };
	m_ParticleProps.ColorEnd = { 0.2f, 0.2f, 0.2f, 0.0f };

	m_ParticleProps.SizeBegin = 0.2f;
	m_ParticleProps.SizeEnd = 0.0f;
	m_ParticleProps.SizeVariation = 0.1f;

	m_ParticleProps.LifeTime = 1.0f;

	m_ParticleProps.Velocity = { 0.0f, 1.0f };
	m_ParticleProps.VelocityVariation = { 1.0f, 1.0f };

	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
				   Yuicy::Application::Get().GetWindow().GetHeight() };
}

void Sandbox2D::OnAttach()
{
	m_CheckerboardTexture = Yuicy::Texture2D::Create("assets/textures/Checkerboard.png");

	m_ActiveScene = Yuicy::CreateRef<Yuicy::Scene>();

	// ==================== 创建相机 ====================
	m_CameraEntity = m_ActiveScene->CreateEntity("Camera");
	m_CameraEntity.AddComponent<Yuicy::CameraComponent>();
	// 将相机向后移动一点以便看到场景
	auto& cameraTransform = m_CameraEntity.GetComponent<Yuicy::TransformComponent>();
	cameraTransform.Translation = { 0.0f, 2.0f, 0.0f };

	// ==================== 创建地面（静态刚体）====================
	m_GroundEntity = m_ActiveScene->CreateEntity("Ground");
	auto& groundTransform = m_GroundEntity.GetComponent<Yuicy::TransformComponent>();
	groundTransform.Translation = { 0.0f, -2.0f, 0.0f };
	groundTransform.Scale = { 10.0f, 0.5f, 1.0f };

	m_GroundEntity.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.3f, 0.3f, 0.3f, 1.0f });

	// 添加静态刚体和碰撞体
	auto& groundRb = m_GroundEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
	groundRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

	auto& groundCollider = m_GroundEntity.AddComponent<Yuicy::BoxCollider2DComponent>();
	groundCollider.Size = { 0.5f, 0.5f };  // 半尺寸，实际尺寸会乘以 Scale

	// ==================== 创建动态方块 ====================
	m_DynamicBox = m_ActiveScene->CreateEntity("Dynamic Box");
	auto& boxTransform = m_DynamicBox.GetComponent<Yuicy::TransformComponent>();
	boxTransform.Translation = { -1.0f, 4.0f, 0.0f };  // 从高处落下
	boxTransform.Scale = { 0.8f, 0.8f, 1.0f };
	boxTransform.Rotation = { 0.0f, 0.0f, glm::radians(15.0f) };  // 初始旋转

	m_DynamicBox.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.8f, 0.2f, 0.3f, 1.0f });

	// 添加动态刚体
	auto& boxRb = m_DynamicBox.AddComponent<Yuicy::Rigidbody2DComponent>();
	boxRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	boxRb.FixedRotation = false;  // 允许旋转

	// 添加碰撞体
	auto& boxCollider = m_DynamicBox.AddComponent<Yuicy::BoxCollider2DComponent>();
	boxCollider.Density = 1.0f;
	boxCollider.Friction = 0.5f;
	boxCollider.Restitution = 0.3f;  // 一点弹性

	// ==================== 创建第二个动态方块 ====================
	auto box2 = m_ActiveScene->CreateEntity("Dynamic Box 2");
	auto& box2Transform = box2.GetComponent<Yuicy::TransformComponent>();
	box2Transform.Translation = { 1.5f, 6.0f, 0.0f };
	box2Transform.Scale = { 0.6f, 0.6f, 1.0f };

	box2.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.2f, 0.8f, 0.3f, 1.0f });

	auto& box2Rb = box2.AddComponent<Yuicy::Rigidbody2DComponent>();
	box2Rb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;

	auto& box2Collider = box2.AddComponent<Yuicy::BoxCollider2DComponent>();
	box2Collider.Density = 0.5f;  // 较轻
	box2Collider.Restitution = 0.6f;  // 更有弹性

	// ==================== 创建第三个动态方块 ====================
	auto box3 = m_ActiveScene->CreateEntity("Dynamic Box 3");
	auto& box3Transform = box3.GetComponent<Yuicy::TransformComponent>();
	box3Transform.Translation = { 0.0f, 8.0f, 0.0f };
	box3Transform.Scale = { 1.0f, 1.0f, 1.0f };

	box3.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.2f, 0.3f, 0.8f, 1.0f });

	auto& box3Rb = box3.AddComponent<Yuicy::Rigidbody2DComponent>();
	box3Rb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;

	auto& box3Collider = box3.AddComponent<Yuicy::BoxCollider2DComponent>();
	box3Collider.Density = 2.0f;  // 较重

	// ==================== 创建斜坡（静态）====================
	auto ramp = m_ActiveScene->CreateEntity("Ramp");
	auto& rampTransform = ramp.GetComponent<Yuicy::TransformComponent>();
	rampTransform.Translation = { 3.0f, -1.0f, 0.0f };
	rampTransform.Scale = { 3.0f, 0.3f, 1.0f };
	rampTransform.Rotation = { 0.0f, 0.0f, glm::radians(25.0f) };  // 倾斜

	ramp.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.5f, 0.4f, 0.3f, 1.0f });

	auto& rampRb = ramp.AddComponent<Yuicy::Rigidbody2DComponent>();
	rampRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;

	ramp.AddComponent<Yuicy::BoxCollider2DComponent>();

	// 初始化视口大小
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

	// ==================== 启动物理模拟 ====================
	m_ActiveScene->OnRuntimeStart();
}

void Sandbox2D::OnDetach()
{
	// 停止物理模拟
	m_ActiveScene->OnRuntimeStop();
}

void Sandbox2D::OnUpdate(Yuicy::Timestep ts)
{
	YUICY_PROFILE_FUNCTION();

	// Update
	// m_CameraController.OnUpdate(ts);

	// 粒子更新
	m_ParticleSystem.OnUpdate(ts);

	Yuicy::Renderer2D::ResetStats();

	// Render
	Yuicy::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Yuicy::RenderCommand::Clear();

	YUICY_PROFILE_SCOPE("Renderer Draw");
	// 使用运行时更新（包含物理模拟）
	m_ActiveScene->OnUpdateRuntime(ts);
}

void Sandbox2D::OnImGuiRender()
{
	YUICY_PROFILE_FUNCTION();
	ImGui::Begin("Physics Demo");

	auto stats = Yuicy::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);

	ImGui::Separator();
	ImGui::Text("Physics Objects:");

	// 显示动态方块的位置信息
	if (m_DynamicBox)
	{
		auto& transform = m_DynamicBox.GetComponent<Yuicy::TransformComponent>();
		ImGui::Text("Red Box Position: (%.2f, %.2f)", transform.Translation.x, transform.Translation.y);
		ImGui::Text("Red Box Rotation: %.2f deg", glm::degrees(transform.Rotation.z));
	}

	ImGui::Separator();
	ImGui::Text("Press 'R' to restart simulation");

	ImGui::End();
}

void Sandbox2D::OnEvent(Yuicy::Event& e)
{
	// m_CameraController.OnEvent(e);

	Yuicy::EventDispatcher dispatcher(e);

	dispatcher.Dispatch<Yuicy::WindowResizeEvent>([this](Yuicy::WindowResizeEvent& e) {
		if (e.GetHeight() == 0.0f)
			return false;

		m_ViewportSize = { (float)e.GetWidth(), (float)e.GetHeight() };
		m_ActiveScene->OnViewportResize(e.GetWidth(), e.GetHeight());
		return false;
		});

	// 按 R 键重启物理模拟
	dispatcher.Dispatch<Yuicy::KeyPressedEvent>([this](Yuicy::KeyPressedEvent& e) {
		if (e.GetKeyCode() == Yuicy::Key::R)
		{
			// 重启物理模拟
			m_ActiveScene->OnRuntimeStop();

			// 重置位置
			if (m_DynamicBox)
			{
				auto& transform = m_DynamicBox.GetComponent<Yuicy::TransformComponent>();
				transform.Translation = { -1.0f, 4.0f, 0.0f };
				transform.Rotation = { 0.0f, 0.0f, glm::radians(15.0f) };
			}

			m_ActiveScene->OnRuntimeStart();
		}
		return false;
		});
}