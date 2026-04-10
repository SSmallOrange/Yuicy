#pragma once

#include "Yuicy/Renderer/Camera.h"
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Events/MouseEvent.h"

#include <glm/glm.hpp>

namespace Yuicy {

	class EditorCamera : public Camera
	{
	public:
		EditorCamera() = default;
		EditorCamera(float aspectRatio, float zoomLevel = 5.0f);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		void SetViewportSize(uint32_t width, uint32_t height);

		const glm::mat4& GetViewMatrix() const { return m_viewMatrix; }
		glm::mat4 GetViewProjection() const { return m_Projection * m_viewMatrix; }

		const glm::vec3& GetPosition() const { return m_position; }

		float GetZoomLevel() const { return m_zoomLevel; }
		void SetZoomLevel(float level);

	private:
		void UpdateProjection();
		void UpdateView();

		bool OnMouseScrolled(MouseScrolledEvent& e);

	private:
		glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
		float m_zoomLevel = 5.0f;
		float m_aspectRatio = 1.778f;
		float m_translationSpeed = 5.0f;

		glm::mat4 m_viewMatrix = glm::mat4(1.0f);

		uint32_t m_viewportWidth = 1280;
		uint32_t m_viewportHeight = 720;
	};

}
