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
	// 加载纹理
	m_CheckerboardTexture = Yuicy::Texture2D::Create("assets/textures/Checkerboard.png");

	m_ActiveScene = Yuicy::CreateRef<Yuicy::Scene>();

	// ==================== 创建相机 ====================
	m_CameraEntity = m_ActiveScene->CreateEntity("Camera");
	m_CameraEntity.AddComponent<Yuicy::CameraComponent>();
	auto& cameraTransform = m_CameraEntity.GetComponent<Yuicy::TransformComponent>();
	cameraTransform.Translation = { 0.0f, 2.0f, 0.0f };

	// ==================== 创建带纹理的地面 ====================
	m_GroundEntity = m_ActiveScene->CreateEntity("Ground");
	auto& groundTransform = m_GroundEntity.GetComponent<Yuicy::TransformComponent>();
	groundTransform.Translation = { 0.0f, -2.0f, 0.0f };
	groundTransform.Scale = { 10.0f, 0.5f, 1.0f };

	// 使用纹理作为地面（带平铺和着色）
	auto& groundSprite = m_GroundEntity.AddComponent<Yuicy::SpriteRendererComponent>();
	groundSprite.Texture = m_CheckerboardTexture;
	groundSprite.TilingFactor = 5.0f;  // 纹理平铺 5 次
	groundSprite.Color = { 0.5f, 0.5f, 0.5f, 1.0f };  // 灰色着色

	auto& groundRb = m_GroundEntity.AddComponent<Yuicy::Rigidbody2DComponent>();
	groundRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	m_GroundEntity.AddComponent<Yuicy::BoxCollider2DComponent>();

	// ==================== 创建带纹理的动态方块（测试翻转）====================
	m_DynamicBox = m_ActiveScene->CreateEntity("Textured Box");
	auto& boxTransform = m_DynamicBox.GetComponent<Yuicy::TransformComponent>();
	boxTransform.Translation = { -1.0f, 4.0f, 0.0f };
	boxTransform.Scale = { 1.0f, 1.0f, 1.0f };
	boxTransform.Rotation = { 0.0f, 0.0f, glm::radians(15.0f) };

	// 使用纹理并测试翻转
	auto& boxSprite = m_DynamicBox.AddComponent<Yuicy::SpriteRendererComponent>();
	boxSprite.Texture = m_CheckerboardTexture;
	boxSprite.Color = { 1.0f, 0.8f, 0.8f, 1.0f };  // 淡红色着色
	boxSprite.FlipX = false;  // 可以在 ImGui 中动态修改

	auto& boxRb = m_DynamicBox.AddComponent<Yuicy::Rigidbody2DComponent>();
	boxRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	boxRb.FixedRotation = false;

	auto& boxCollider = m_DynamicBox.AddComponent<Yuicy::BoxCollider2DComponent>();
	boxCollider.Density = 1.0f;
	boxCollider.Friction = 0.5f;
	boxCollider.Restitution = 0.3f;

	// ==================== 创建纯色方块 ====================
	auto colorBox = m_ActiveScene->CreateEntity("Color Box");
	auto& colorBoxTransform = colorBox.GetComponent<Yuicy::TransformComponent>();
	colorBoxTransform.Translation = { 1.5f, 6.0f, 0.0f };
	colorBoxTransform.Scale = { 0.8f, 0.8f, 1.0f };

	// 纯色，无纹理
	colorBox.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.2f, 0.8f, 0.3f, 1.0f });

	auto& colorBoxRb = colorBox.AddComponent<Yuicy::Rigidbody2DComponent>();
	colorBoxRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	colorBox.AddComponent<Yuicy::BoxCollider2DComponent>();

	// ==================== 创建半透明方块 ====================
	auto transparentBox = m_ActiveScene->CreateEntity("Transparent Box");
	auto& transparentTransform = transparentBox.GetComponent<Yuicy::TransformComponent>();
	transparentTransform.Translation = { 0.0f, 8.0f, 0.0f };
	transparentTransform.Scale = { 1.2f, 1.2f, 1.0f };

	// 带纹理的半透明
	auto& transparentSprite = transparentBox.AddComponent<Yuicy::SpriteRendererComponent>();
	transparentSprite.Texture = m_CheckerboardTexture;
	transparentSprite.Color = { 0.3f, 0.3f, 1.0f, 0.7f };  // 半透明蓝色

	auto& transparentRb = transparentBox.AddComponent<Yuicy::Rigidbody2DComponent>();
	transparentRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	transparentBox.AddComponent<Yuicy::BoxCollider2DComponent>();

	// ==================== 创建斜坡 ====================
	auto ramp = m_ActiveScene->CreateEntity("Ramp");
	auto& rampTransform = ramp.GetComponent<Yuicy::TransformComponent>();
	rampTransform.Translation = { 3.0f, -1.0f, 0.0f };
	rampTransform.Scale = { 3.0f, 0.3f, 1.0f };
	rampTransform.Rotation = { 0.0f, 0.0f, glm::radians(25.0f) };

	auto& rampSprite = ramp.AddComponent<Yuicy::SpriteRendererComponent>();
	rampSprite.Color = { 0.6f, 0.4f, 0.2f, 1.0f };  // 棕色

	auto& rampRb = ramp.AddComponent<Yuicy::Rigidbody2DComponent>();
	rampRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Static;
	ramp.AddComponent<Yuicy::BoxCollider2DComponent>();

	// 初始化视口大小
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

	// 启动物理模拟
	m_ActiveScene->OnRuntimeStart();
}

void Sandbox2D::OnDetach()
{
	m_ActiveScene->OnRuntimeStop();
}

void Sandbox2D::OnUpdate(Yuicy::Timestep ts)
{
	YUICY_PROFILE_FUNCTION();

	m_ParticleSystem.OnUpdate(ts);
	Yuicy::Renderer2D::ResetStats();

	Yuicy::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Yuicy::RenderCommand::Clear();

	YUICY_PROFILE_SCOPE("Renderer Draw");
	m_ActiveScene->OnUpdateRuntime(ts);
}

void Sandbox2D::OnImGuiRender()
{
	YUICY_PROFILE_FUNCTION();
	ImGui::Begin("Sprite Demo");

	auto stats = Yuicy::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);

	ImGui::Separator();
	ImGui::Text("Sprite Properties:");

	// 动态修改精灵属性
	if (m_DynamicBox)
	{
		auto& sprite = m_DynamicBox.GetComponent<Yuicy::SpriteRendererComponent>();
		auto& transform = m_DynamicBox.GetComponent<Yuicy::TransformComponent>();

		ImGui::Text("Textured Box:");
		ImGui::ColorEdit4("Tint Color", glm::value_ptr(sprite.Color));
		ImGui::Checkbox("Flip X", &sprite.FlipX);
		ImGui::Checkbox("Flip Y", &sprite.FlipY);
		ImGui::DragFloat("Tiling Factor", &sprite.TilingFactor, 0.1f, 0.1f, 10.0f);

		ImGui::Text("Position: (%.2f, %.2f)", transform.Translation.x, transform.Translation.y);
		ImGui::Text("Rotation: %.2f deg", glm::degrees(transform.Rotation.z));
	}

	ImGui::Separator();
	ImGui::Text("Press 'R' to restart simulation");

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