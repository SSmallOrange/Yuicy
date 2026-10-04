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

	Yuicy::ParticleSystem m_ParticleSystem;
	Yuicy::ParticleProps m_ParticleProps;

	// SceneContext 只持有弱引用，资源管理器的生命周期由这里保证
	Yuicy::Ref<Yuicy::RuntimeAssetManager> m_AssetManager;
	Yuicy::Ref<Yuicy::Scene> m_ActiveScene;

	glm::vec2 m_ViewportSize = { 1280.0f, 720.0f };
};