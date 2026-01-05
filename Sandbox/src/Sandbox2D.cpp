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
// 23, 0 (16, 16)
void Sandbox2D::OnAttach()
{
	m_CheckerboardTexture = Yuicy::Texture2D::Create("assets/textures/Checkerboard.png");
	m_PlayerSheet = Yuicy::Texture2D::Create("assets/textures/tilemap_packed.png");

	m_ActiveScene = Yuicy::CreateRef<Yuicy::Scene>();

	// ==================== 创建带动画的玩家 ====================
	m_Player = m_ActiveScene->CreateEntity("Player");
	auto& playerTransform = m_Player.GetComponent<Yuicy::TransformComponent>();
	playerTransform.Translation = { 0.0f, 2.0f, 0.0f };
	playerTransform.Scale = { 1.0f, 1.0f, 1.0f };

	// 添加精灵渲染组件（动画会自动更新 SubTexture）
	m_Player.AddComponent<Yuicy::SpriteRendererComponent>();

	// 添加动画组件
	auto& anim = m_Player.AddComponent<Yuicy::AnimationComponent>();

	// 创建待机动画 - 假设精灵图集第 0 行是待机动画
	Yuicy::AnimationClip idleClip("Idle", 0.15f, true);
	idleClip.AddFramesFromSheet(m_PlayerSheet, { 23, 0 }, 1, { 16, 16 }, { 1.0f, 1.0f }, true);
	anim.AddClip(idleClip);

	// 创建跳跃动画 - 假设精灵图集第 2 行是跳跃动画
	Yuicy::AnimationClip jumpClip("Jump", 0.1f, false);  // 不循环
	jumpClip.AddFramesFromSheet(m_PlayerSheet, { 23, 2 }, 4, { 16, 16 }, { 1.0f, 1.0f }, true);
	anim.AddClip(jumpClip);

	// 默认播放待机动画
	anim.Play("Idle");

	// 添加物理组件
	auto& playerRb = m_Player.AddComponent<Yuicy::Rigidbody2DComponent>();
	playerRb.Type = Yuicy::Rigidbody2DComponent::BodyType::Dynamic;
	playerRb.FixedRotation = true;

	auto& playerCollider = m_Player.AddComponent<Yuicy::BoxCollider2DComponent>();
	playerCollider.Size = { 0.4f, 0.5f };  // 碰撞体略小于精灵

	// 绑定脚本
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

	// 天气
	m_weatherSystem.SetWeather(Yuicy::WeatherType::Snow, Yuicy::WeatherIntensity::Normal);

	// 稀疏萤火虫配置
	auto sparseFireflies = Yuicy::WeatherPresets::Fireflies();
	sparseFireflies.name = "SparseFireflies";
	sparseFireflies.particles.spawnRate = 1.5f;       // 每秒 1.5 个
	sparseFireflies.particles.particleLifetime = 25.0f; // 活 25 秒
	sparseFireflies.intensity = 1.0f;
	Yuicy::WeatherPresets::RegisterPreset("SparseFireflies", sparseFireflies);

	m_weatherSystem.TransitionTo("SparseFireflies");

	// 初始化
	m_ViewportSize = { Yuicy::Application::Get().GetWindow().GetWidth(),
					   Yuicy::Application::Get().GetWindow().GetHeight() };
	m_ActiveScene->OnViewportResize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);

	// 启动场景
	m_ActiveScene->OnRuntimeStart();

	// 设置相机跟随目标
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

	m_weatherSystem.OnUpdate(ts);

	Yuicy::Renderer2D::ResetStats();

	Yuicy::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.2f, 1 });
	Yuicy::RenderCommand::Clear();

	m_ActiveScene->OnUpdateRuntime(ts);

	if (m_CameraEntity && m_weatherSystem.IsActive())
	{
		auto& cameraComp = m_CameraEntity.GetComponent<Yuicy::CameraComponent>();
		auto& transform = m_CameraEntity.GetComponent<Yuicy::TransformComponent>();

		// 计算视口尺寸（世界坐标）
		float orthoSize = cameraComp.Camera.GetOrthographicSize();
		float aspectRatio = m_ViewportSize.x / m_ViewportSize.y;
		glm::vec2 viewportWorldSize = { orthoSize * aspectRatio, orthoSize };

		Yuicy::Renderer2D::BeginScene(cameraComp.Camera, transform.GetTransform());
		m_weatherSystem.OnRender(
			{ transform.Translation.x, transform.Translation.y },
			viewportWorldSize
		);
		Yuicy::Renderer2D::EndScene();
	}
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
	ImGui::Text("Weather System:");

	// 预设选择下拉框
	static int selectedPreset = 0;
	std::vector<std::string> presetNames = Yuicy::WeatherPresets::GetAllPresetNames();
	presetNames.insert(presetNames.begin(), "None");  // 添加无天气选项

	if (ImGui::BeginCombo("Preset", presetNames[selectedPreset].c_str()))
	{
		for (int i = 0; i < presetNames.size(); i++)
		{
			bool isSelected = (selectedPreset == i);
			if (ImGui::Selectable(presetNames[i].c_str(), isSelected))
			{
				selectedPreset = i;
				if (i == 0)
					m_weatherSystem.FadeOut(2.0f);
				else
					m_weatherSystem.TransitionTo(presetNames[i]);
			}
			if (isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	// 当前状态显示
	ImGui::Text("Current: %s", m_weatherSystem.GetCurrentWeatherName().c_str());
	if (m_weatherSystem.IsTransitioning())
	{
		ImGui::Text("Transitioning: %.0f%%", m_weatherSystem.GetTransitionProgress() * 100.0f);
	}

	// 实时调参
	float intensity = m_weatherSystem.GetIntensity();
	if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 3.0f))
	{
		m_weatherSystem.SetIntensity(intensity);
	}

	float wind = m_weatherSystem.GetWindStrength();
	if (ImGui::SliderFloat("Wind", &wind, -1.0f, 1.0f))
	{
		m_weatherSystem.SetWindStrength(wind);
	}

	// 快速切换按钮
	if (ImGui::Button("Rain")) m_weatherSystem.TransitionTo(Yuicy::WeatherType::Rain);
	ImGui::SameLine();
	if (ImGui::Button("Snow")) m_weatherSystem.TransitionTo(Yuicy::WeatherType::Snow);
	ImGui::SameLine();
	if (ImGui::Button("Storm")) m_weatherSystem.TransitionTo("Storm");

	if (ImGui::Button("Leaves")) m_weatherSystem.TransitionTo("FallingLeaves");
	ImGui::SameLine();
	if (ImGui::Button("Fireflies")) m_weatherSystem.TransitionTo("Fireflies");
	ImGui::SameLine();
	if (ImGui::Button("Clear")) m_weatherSystem.FadeOut();

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