#include "pch.h"
#include "EditorCamera.h"

#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Yuicy {

	EditorCamera::EditorCamera(float aspectRatio, float zoomLevel)
		: m_aspectRatio(aspectRatio), m_zoomLevel(zoomLevel)
	{
		UpdateProjection();
		UpdateView();
	}

	void EditorCamera::OnUpdate(Timestep ts)
	{
		bool altPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
		if (!altPressed)
		{
			// 编辑器相机移动需要 Alt 作为修饰键，避免与 Gizmo 快捷键冲突。
			m_translationSpeed = m_zoomLevel;
			return;
		}

		if (Input::IsKeyPressed(Key::A))
		{
			m_position.x -= m_translationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::D))
		{
			m_position.x += m_translationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::W))
		{
			m_position.y += m_translationSpeed * ts;
		}
		if (Input::IsKeyPressed(Key::S))
		{
			m_position.y -= m_translationSpeed * ts;
		}

		// 移动速度受缩放级别控制
		m_translationSpeed = m_zoomLevel;

		UpdateView();
	}

	void EditorCamera::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<MouseScrolledEvent>(
			[this](MouseScrolledEvent& event) { return OnMouseScrolled(event); }
		);
	}

	void EditorCamera::SetViewportSize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		if (m_viewportWidth == width && m_viewportHeight == height)
			return;

		m_viewportWidth = width;
		m_viewportHeight = height;
		m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);

		UpdateProjection();
	}

	void EditorCamera::SetZoomLevel(float level)
	{
		m_zoomLevel = glm::max(level, 0.25f);
		UpdateProjection();
	}

	void EditorCamera::UpdateProjection()
	{
		// 正交投影
		float left   = -m_aspectRatio * m_zoomLevel;
		float right  =  m_aspectRatio * m_zoomLevel;
		float bottom = -m_zoomLevel;
		float top    =  m_zoomLevel;

		m_Projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
	}

	void EditorCamera::UpdateView()
	{
		// 2D 视图矩阵：仅平移（无旋转）
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position);
		m_viewMatrix = glm::inverse(transform);
	}

	bool EditorCamera::OnMouseScrolled(MouseScrolledEvent& e)
	{
		m_zoomLevel -= e.GetYOffset() * 0.25f;
		m_zoomLevel = glm::max(m_zoomLevel, 0.25f);
		UpdateProjection();
		return false;
	}

}
