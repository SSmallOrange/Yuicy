#include "pch.h"
#include "Platform/Windows/WindowsWindowOverlay.h"
#include "Yuicy/Core/Application.h"

#include <imgui.h>
#include <GLFW/glfw3.h>

namespace Yuicy {

	WindowsWindowOverlay::WindowsWindowOverlay()
	{
	}

	WindowsWindowOverlay::~WindowsWindowOverlay()
	{
		Shutdown();
	}

	void WindowsWindowOverlay::Init()
	{
		if (m_initialized)
			return;

		m_window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());

		// Load button textures if specified
		if (!m_config.closeButtonTexture.empty())
			m_closeButtonTex = Texture2D::Create(m_config.closeButtonTexture);

		if (!m_config.minimizeButtonTexture.empty())
			m_minimizeButtonTex = Texture2D::Create(m_config.minimizeButtonTexture);

		if (!m_config.maximizeButtonTexture.empty())
			m_maximizeButtonTex = Texture2D::Create(m_config.maximizeButtonTexture);

		m_initialized = true;
	}

	void WindowsWindowOverlay::Shutdown()
	{
		m_closeButtonTex.reset();
		m_minimizeButtonTex.reset();
		m_maximizeButtonTex.reset();
		m_initialized = false;
	}

	void WindowsWindowOverlay::OnImGuiRender()
	{
		if (!m_enabled || !m_initialized)
			return;

		RenderTitleBar();

		if (m_config.enableDragging)
			HandleWindowDragging();
	}

	void WindowsWindowOverlay::RenderTitleBar()
	{
		ImGuiViewport* mainViewport = ImGui::GetMainViewport();

		ImGuiWindowFlags windowFlags = 
			ImGuiWindowFlags_NoDecoration | 
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize | 
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoDocking;

		// Lock to main viewport
		ImGui::SetNextWindowViewport(mainViewport->ID);

		// Position at top of main viewport (use viewport's screen position)
		ImGui::SetNextWindowPos(mainViewport->Pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(mainViewport->Size.x, m_config.titleBarHeight), ImGuiCond_Always);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(
			m_config.titleBarColor.r, 
			m_config.titleBarColor.g, 
			m_config.titleBarColor.b, 
			m_config.titleBarColor.a));

		ImGui::Begin("##TitleBar", nullptr, windowFlags);

		// Optional title text
		if (m_config.showTitle && !m_config.windowTitle.empty())
		{
			ImGui::SetCursorPos(ImVec2(10.0f, (m_config.titleBarHeight - ImGui::GetTextLineHeight()) / 2.0f));
			ImGui::Text("%s", m_config.windowTitle.c_str());
		}

		// Calculate button positions (right-aligned)
		float buttonY = (m_config.titleBarHeight - m_config.buttonSize) / 2.0f;
		float currentX = mainViewport->Size.x - m_config.buttonPadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		// Close button (rightmost)
		if (m_config.showCloseButton)
		{
			currentX -= m_config.buttonSize;
			ImGui::SetCursorPos(ImVec2(currentX, buttonY));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
				m_config.closeButtonHoverColor.r,
				m_config.closeButtonHoverColor.g,
				m_config.closeButtonHoverColor.b,
				m_config.closeButtonHoverColor.a));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

			bool clicked = false;
			if (m_closeButtonTex)
			{
				clicked = ImGui::ImageButton("##close", 
					(ImTextureID)(uintptr_t)m_closeButtonTex->GetRendererID(),
					ImVec2(m_config.buttonSize, m_config.buttonSize),
					ImVec2(0, 1), ImVec2(1, 0));
			}
			else
			{
				clicked = ImGui::Button("X", ImVec2(m_config.buttonSize, m_config.buttonSize));
			}

			if (clicked)
				Application::Get().GetWindow().Close();

			ImGui::PopStyleColor(3);
			currentX -= m_config.buttonPadding;
		}

		// Maximize button
		if (m_config.showMaximizeButton)
		{
			currentX -= m_config.buttonSize;
			ImGui::SetCursorPos(ImVec2(currentX, buttonY));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
				m_config.buttonHoverColor.r,
				m_config.buttonHoverColor.g,
				m_config.buttonHoverColor.b,
				m_config.buttonHoverColor.a));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));

			bool clicked = false;
			bool isMaximized = Application::Get().GetWindow().IsMaximized();

			if (m_maximizeButtonTex)
			{
				clicked = ImGui::ImageButton("##maximize",
					(ImTextureID)(uintptr_t)m_maximizeButtonTex->GetRendererID(),
					ImVec2(m_config.buttonSize, m_config.buttonSize),
					ImVec2(0, 1), ImVec2(1, 0));
			}
			else
			{
				clicked = ImGui::Button(isMaximized ? "O" : "[]", ImVec2(m_config.buttonSize, m_config.buttonSize));
			}

			if (clicked)
			{
				if (isMaximized)
					Application::Get().GetWindow().Restore();
				else
					Application::Get().GetWindow().Maximize();
			}

			ImGui::PopStyleColor(3);
			currentX -= m_config.buttonPadding;
		}

		// Minimize button
		if (m_config.showMinimizeButton)
		{
			currentX -= m_config.buttonSize;
			ImGui::SetCursorPos(ImVec2(currentX, buttonY));

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
				m_config.buttonHoverColor.r,
				m_config.buttonHoverColor.g,
				m_config.buttonHoverColor.b,
				m_config.buttonHoverColor.a));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.45f, 1.0f));

			bool clicked = false;
			if (m_minimizeButtonTex)
			{
				clicked = ImGui::ImageButton("##minimize",
					(ImTextureID)(uintptr_t)m_minimizeButtonTex->GetRendererID(),
					ImVec2(m_config.buttonSize, m_config.buttonSize),
					ImVec2(0, 1), ImVec2(1, 0));
			}
			else
			{
				clicked = ImGui::Button("-", ImVec2(m_config.buttonSize, m_config.buttonSize));
			}

			if (clicked)
				Application::Get().GetWindow().Minimize();

			ImGui::PopStyleColor(3);
		}

		ImGui::PopStyleVar(2);  // FramePadding, ItemSpacing
		ImGui::End();
		ImGui::PopStyleColor();  // WindowBg
		ImGui::PopStyleVar(2);   // WindowPadding, WindowBorderSize
	}

	void WindowsWindowOverlay::HandleWindowDragging()
	{
		// Use GLFW for cursor position (window-relative coordinates)
		double cursorX, cursorY;
		glfwGetCursorPos(m_window, &cursorX, &cursorY);

		// 检查鼠标是否在标题栏区域内
		float buttonAreaWidth = m_config.buttonPadding * 4 + m_config.buttonSize * 3;
		int winWidth, winHeight;
		glfwGetWindowSize(m_window, &winWidth, &winHeight);

		bool inTitleBar = cursorY >= 0 && 
		                  cursorY < m_config.titleBarHeight &&
		                  cursorX >= 0 &&
		                  cursorX < winWidth - buttonAreaWidth;

		int mouseButton = glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT);

		if (inTitleBar && mouseButton == GLFW_PRESS && !m_isDragging)
		{
			m_isDragging = true;
			
			// Store cursor position in screen coordinates
			int winX, winY;
			glfwGetWindowPos(m_window, &winX, &winY);
			m_dragStartPos = { (float)(winX + cursorX), (float)(winY + cursorY) };
			m_windowStartPos = { (float)winX, (float)winY };
		}

		if (m_isDragging)
		{
			if (mouseButton == GLFW_PRESS)
			{
				// Get current cursor screen position
				int winX, winY;
				glfwGetWindowPos(m_window, &winX, &winY);
				float screenCursorX = (float)(winX + cursorX);
				float screenCursorY = (float)(winY + cursorY);

				float deltaX = screenCursorX - m_dragStartPos.x;
				float deltaY = screenCursorY - m_dragStartPos.y;

				int newX = (int)(m_windowStartPos.x + deltaX);
				int newY = (int)(m_windowStartPos.y + deltaY);

				glfwSetWindowPos(m_window, newX, newY);
			}
			else
			{
				m_isDragging = false;
			}
		}
	}

}
