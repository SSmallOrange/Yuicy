#include "pch.h"
#include "EditorCamera.h"

#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Core/MouseCodes.h"

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
		// MMB 拖拽平移
		bool mmbPressed = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);
		if (mmbPressed)
		{
			if (!m_isPanning)
			{
				// 平移开始：记录鼠标下的世界坐标作为锚点
				m_isPanning = true;
				m_panAnchorWorld = ScreenToWorld(m_viewportMouseX, m_viewportMouseY);
			}
			else
			{
				// 平移中：调整相机位置使锚点保持在鼠标下
				glm::vec2 currentWorldAtMouse = ScreenToWorld(m_viewportMouseX, m_viewportMouseY);
				glm::vec2 delta = m_panAnchorWorld - currentWorldAtMouse;
				m_position.x += delta.x;
				m_position.y += delta.y;
				UpdateView();
			}

			m_translationSpeed = m_zoomLevel;
			return;
		}
		else
		{
			m_isPanning = false;
		}

		// Alt + WASD 键盘移动
		bool altPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
		if (!altPressed)
		{
			m_translationSpeed = m_zoomLevel;
			return;
		}

		// Shift 加速
		float speedMultiplier = 1.0f;
		if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
			speedMultiplier = 3.0f;

		float moveSpeed = m_translationSpeed * speedMultiplier;

		if (Input::IsKeyPressed(Key::A))
			m_position.x += moveSpeed * ts;
		if (Input::IsKeyPressed(Key::D))
			m_position.x -= moveSpeed * ts;
		if (Input::IsKeyPressed(Key::W))
			m_position.y -= moveSpeed * ts;
		if (Input::IsKeyPressed(Key::S))
			m_position.y += moveSpeed * ts;

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

	void EditorCamera::SetViewportMousePosition(float x, float y)
	{
		m_viewportMouseX = x;
		m_viewportMouseY = y;
	}

	void EditorCamera::SetPosition(const glm::vec3& position)
	{
		m_position = position;
		UpdateView();
	}

	void EditorCamera::SetZoomLevel(float level)
	{
		m_zoomLevel = glm::max(level, 0.25f);
		UpdateProjection();
	}

	glm::vec2 EditorCamera::ScreenToWorld(float viewportX, float viewportY) const
	{
		if (m_viewportWidth == 0 || m_viewportHeight == 0)
			return { m_position.x, m_position.y };

		// 视口坐标 → NDC [-1, 1]（Y 翻转）
		float ndcX = (viewportX / static_cast<float>(m_viewportWidth)) * 2.0f - 1.0f;
		float ndcY = 1.0f - (viewportY / static_cast<float>(m_viewportHeight)) * 2.0f;

		// NDC → 世界坐标
		float worldX = m_position.x + ndcX * m_aspectRatio * m_zoomLevel;
		float worldY = m_position.y + ndcY * m_zoomLevel;

		return { worldX, worldY };
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
		// 缩放前鼠标下的世界坐标
		glm::vec2 worldBeforeZoom = ScreenToWorld(m_viewportMouseX, m_viewportMouseY);

		// 应用缩放
		m_zoomLevel -= e.GetYOffset() * 0.25f;
		m_zoomLevel = glm::max(m_zoomLevel, 0.25f);
		UpdateProjection();

		// 缩放后鼠标下的世界坐标
		glm::vec2 worldAfterZoom = ScreenToWorld(m_viewportMouseX, m_viewportMouseY);

		// 调整相机位置，使鼠标下的世界点保持不变
		m_position.x += worldBeforeZoom.x - worldAfterZoom.x;
		m_position.y += worldBeforeZoom.y - worldAfterZoom.y;
		UpdateView();

		return false;
	}

}
