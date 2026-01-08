#pragma once
#include "Yuicy.h"

class Sandbox2D : public Yuicy::Layer
{
public:
	Sandbox2D();
	virtual ~Sandbox2D() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Yuicy::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Yuicy::Event& e) override;

private:
	Yuicy::OrthographicCameraController m_CameraController;

	Yuicy::Ref<Yuicy::Texture2D> m_CheckerboardTexture;
	Yuicy::Ref<Yuicy::Texture2D> m_PlayerSheet;

	Yuicy::ParticleSystem m_ParticleSystem;
	Yuicy::ParticleProps m_ParticleProps;

	Yuicy::Ref<Yuicy::Scene> m_ActiveScene;

	// 实体引用
	Yuicy::Entity m_Player;
	Yuicy::Entity m_CameraEntity;
	Yuicy::Entity m_GroundEntity;

	glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };

	// 天气
	Yuicy::WeatherSystem m_weatherSystem{ 3000 };
	// FrameBuffer
	Yuicy::Ref<Yuicy::Framebuffer> m_framebuffer;

	Yuicy::PostProcessing m_postProcessing;
};