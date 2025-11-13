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

	// Temp
	Yuicy::Ref<Yuicy::VertexArray> m_SquareVA;
	Yuicy::Ref<Yuicy::Shader> m_FlatColorShader;

	Yuicy::Ref<Yuicy::Texture2D> m_CheckerboardTexture;

	glm::vec4 m_SquareColor = { 0.2f, 0.3f, 0.8f, 1.0f };
};