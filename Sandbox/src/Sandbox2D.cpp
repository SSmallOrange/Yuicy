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
}

void Sandbox2D::OnAttach()
{
	m_CheckerboardTexture = Yuicy::Texture2D::Create("assets/textures/Checkerboard.png");

	m_ActiveScene = Yuicy::CreateRef<Yuicy::Scene>();

	glm::mat4 transformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 1.0f));

	auto square = m_ActiveScene->CreateEntity("Green Square");
	square.GetComponent<Yuicy::TransformComponent>().Transform = transformMatrix;
	square.AddComponent<Yuicy::SpriteRendererComponent>(glm::vec4{ 0.0f, 1.0f, 0.0f, 1.0f });

	m_SquareEntity = square;
}

void Sandbox2D::OnDetach()
{
}

void Sandbox2D::OnUpdate(Yuicy::Timestep ts)
{
	YUICY_PROFILE_FUNCTION();

	// Update
	m_CameraController.OnUpdate(ts);

	// 粒子更新
	m_ParticleSystem.OnUpdate(ts);

	Yuicy::Renderer2D::ResetStats();

	// Render
	Yuicy::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
	Yuicy::RenderCommand::Clear();
	static float rotation = 0.0f;
	rotation += ts * 50.0f;

	YUICY_PROFILE_SCOPE("Renderer Draw");
	Yuicy::Renderer2D::BeginScene(m_CameraController.GetCamera());

	// Update scene
	m_ActiveScene->OnUpdate(ts);
	Yuicy::Renderer2D::EndScene();

// 	// Draw
// 	Yuicy::Renderer2D::BeginScene(m_CameraController.GetCamera());
// 	// Yuicy::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1280.0f / 720.0f, 1280.0f / 720.0f }, { 0.8f, 0.2f, 0.3f, 1.0f });
// 	Yuicy::Renderer2D::DrawQuad({ -1.0f, 0.0f }, { 0.8f, 0.8f }, { 0.8f, 0.2f, 0.3f, 1.0f });
// 	Yuicy::Renderer2D::DrawQuad({ 0.5f, -0.5f }, { 0.5f, 0.75f }, { 0.2f, 0.3f, 0.8f, 1.0f });
// 	Yuicy::Renderer2D::DrawQuad({ 0.0f, 0.0f, -0.1f }, { 20.0f, 20.0f }, m_CheckerboardTexture, 10.0f);
// 	Yuicy::Renderer2D::DrawRotatedQuad({ -2.0f, 0.0f, 0.0f }, { 0.5f, 0.5f }, glm::radians(rotation), m_CheckerboardTexture, 1.0f);
// 	Yuicy::Renderer2D::EndScene();
// 
// 	Yuicy::Renderer2D::BeginScene(m_CameraController.GetCamera());
// 	for (float y = -5.0f; y < 5.0f; y += 0.5f)
// 	{
// 		for (float x = -5.0f; x < 5.0f; x += 0.5f)
// 		{
// 			glm::vec4 color = { (x + 5.0f) / 10.0f, 0.4f, (y + 5.0f) / 10.0f, 0.7f };
// 			Yuicy::Renderer2D::DrawQuad({ x, y }, { 0.45f, 0.45f }, color);
// 		}
// 	}
// 
// 	// 粒子渲染
// 	m_ParticleSystem.OnRender();

//	Yuicy::Renderer2D::EndScene();
}

void Sandbox2D::OnImGuiRender()
{
	YUICY_PROFILE_FUNCTION();
	ImGui::Begin("Settings");

	auto stats = Yuicy::Renderer2D::GetStats();
	ImGui::Text("Renderer2D Stats:");
	ImGui::Text("Draw Calls: %d", stats.DrawCalls);
	ImGui::Text("Quads: %d", stats.QuadCount);
	ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
	ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

	if (m_SquareEntity)
	{
		ImGui::Separator();
		auto& tag = m_SquareEntity.GetComponent<Yuicy::TagComponent>().Tag;
		ImGui::Text("%s", tag.c_str());
		ImGui::Separator();
		auto& squareColor = m_SquareEntity.GetComponent<Yuicy::SpriteRendererComponent>().Color;
		ImGui::ColorEdit4("Square Color", glm::value_ptr(squareColor));
		ImGui::Separator();
	}

	ImGui::End();
}

void Sandbox2D::OnEvent(Yuicy::Event& e)
{
	m_CameraController.OnEvent(e);

	Yuicy::EventDispatcher dispatcher(e);
	dispatcher.Dispatch<Yuicy::MouseButtonPressedEvent>([this](Yuicy::MouseButtonPressedEvent& e) {
		if (e.GetMouseButton() == Yuicy::Mouse::ButtonLeft)
		{
			auto [x, y] = Yuicy::Input::GetMousePosition();
			auto width = Yuicy::Application::Get().GetWindow().GetWidth();
			auto height = Yuicy::Application::Get().GetWindow().GetHeight();

			auto bounds = m_CameraController.GetBounds();

			auto pos = m_CameraController.GetCamera().GetPosition();

			x = (x / width) * bounds.GetWidth() - bounds.GetWidth() * 0.5f;		// 屏幕原点与NOC原点位置不同
			y = bounds.GetHeight() * 0.5f - (y / height) * bounds.GetHeight();	// Y轴方向也不同
			m_ParticleProps.Position = { x + pos.x, y + pos.y };

			// 每次点击发射 10 个粒子
			for (int i = 0; i < 10; ++i)
				m_ParticleSystem.Emit(m_ParticleProps);
		}
		return false;
	});
}