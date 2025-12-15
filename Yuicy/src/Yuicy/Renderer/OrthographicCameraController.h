#pragma once
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Renderer/OrthographicCamera.h"

namespace Yuicy {

	struct OrthographicCameraBounds
	{
		float Left, Right;
		float Bottom, Top;

		float GetWidth() { return Right - Left; }
		float GetHeight() { return Top - Bottom; }
	};

	class OrthographicCameraController
	{
	public:
		// H: 2 * aspectRatio  W: 2 * aspectRatio * ZoomLevel
		OrthographicCameraController(float aspectRatio, bool rotation = false);

		void OnUpdate(Timestep ts);
		void OnEvent(Event& e);

		OrthographicCamera& GetCamera() { return m_Camera; }
		const OrthographicCamera& GetCamera() const { return m_Camera; }
		float GetZoomLevel() const { return m_ZoomLevel; }
		void SetZoomLevel(float level) { m_ZoomLevel = level;  CalculateView(); }

		const OrthographicCameraBounds& GetBounds() const { return m_Bounds; }

	private:
		void CalculateView();

	private:
		bool OnMouseScrolled(MouseScrolledEvent& e);
		bool OnWindowResized(WindowResizeEvent& e);

	private:
		float m_AspectRatio;
		float m_ZoomLevel = 1.0f;

		OrthographicCameraBounds m_Bounds;		// Па»ъ±ЯЅз
		OrthographicCamera m_Camera;

		bool m_Rotation;

		glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
		float m_CameraRotation = 0.0f;
		float m_CameraTranslationSpeed = 5.0f, m_CameraRotationSpeed = 180.0f;
	};

}
