#include "pch.h"
#include "Platform/GLFW/GlfwWindow.h"

#include "Yuicy/Renderer/GraphicsContext.h"
#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Events/KeyEvent.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>

namespace Yuicy {

	// TODO: 与 WindowsWindow.cpp 中的同名计数合并
	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		YUICY_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	GlfwWindow::GlfwWindow(const WindowProps& props)
	{
		YUICY_PROFILE_FUNCTION();

		Init(props);
	}

	GlfwWindow::~GlfwWindow()
	{
		YUICY_PROFILE_FUNCTION();

		Shutdown();
	}

	void GlfwWindow::Init(const WindowProps& props)
	{
		YUICY_PROFILE_FUNCTION();

		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;

		YUICY_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (s_GLFWWindowCount == 0)
		{
			YUICY_PROFILE_SCOPE("glfwInit");
			int success = glfwInit();
			YUICY_CORE_ASSERT(success, "Could not initialize GLFW!");
			glfwSetErrorCallback(GLFWErrorCallback);
		}

		{
			YUICY_PROFILE_SCOPE("glfwCreateWindow");
			// 无边框窗口的拖拽依赖平台实现（如 Win32 WM_NCHITTEST），这里没有，无边框会导致窗口无法移动
			glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
			GraphicsContext::SetWindowHints();
			m_Window = glfwCreateWindow((int)props.Width, (int)props.Height, m_Data.Title.c_str(), nullptr, nullptr);
			// 失败原因（如驱动不支持请求的 GL 版本）已由 GLFWErrorCallback 输出
			YUICY_CORE_ASSERT(m_Window, "Could not create GLFW window!");
			++s_GLFWWindowCount;
		}

		m_Context = GraphicsContext::Create(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(true);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallback(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, true);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
			case GLFW_PRESS:
			{
				MouseButtonPressedEvent event(button);
				data.EventCallback(event);
				break;
			}
			case GLFW_RELEASE:
			{
				MouseButtonReleasedEvent event(button);
				data.EventCallback(event);
				break;
			}
			}
		});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});
	}

	void GlfwWindow::Shutdown()
	{
		YUICY_PROFILE_FUNCTION();

		if (m_CustomCursor)
		{
			glfwDestroyCursor(m_CustomCursor);
			m_CustomCursor = nullptr;
		}

		glfwDestroyWindow(m_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void GlfwWindow::OnUpdate()
	{
		YUICY_PROFILE_FUNCTION();

		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void GlfwWindow::SetVSync(bool enabled)
	{
		YUICY_PROFILE_FUNCTION();

		glfwSwapInterval(enabled ? 1 : 0);
		m_Data.VSync = enabled;
	}

	bool GlfwWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	void GlfwWindow::SetCursor(const std::string& imagePath, int hotspotX, int hotspotY)
	{
		YUICY_PROFILE_FUNCTION();

		if (m_CustomCursor)
		{
			glfwDestroyCursor(m_CustomCursor);
			m_CustomCursor = nullptr;
		}

		int width = 0, height = 0, channels = 0;
		stbi_set_flip_vertically_on_load(0);
		unsigned char* pixels = stbi_load(imagePath.c_str(), &width, &height, &channels, 4);

		if (!pixels)
		{
			YUICY_CORE_ERROR("Failed to load cursor image: {0}", imagePath);
			return;
		}

		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = pixels;

		m_CustomCursor = glfwCreateCursor(&image, hotspotX, hotspotY);
		stbi_image_free(pixels);

		if (m_CustomCursor)
		{
			glfwSetCursor(m_Window, m_CustomCursor);
			YUICY_CORE_INFO("Custom cursor set: {0}", imagePath);
		}
		else
		{
			YUICY_CORE_ERROR("Failed to create cursor from image: {0}", imagePath);
		}
	}

	void GlfwWindow::ResetCursor()
	{
		YUICY_PROFILE_FUNCTION();

		if (m_CustomCursor)
		{
			glfwDestroyCursor(m_CustomCursor);
			m_CustomCursor = nullptr;
		}

		glfwSetCursor(m_Window, nullptr);
	}

	void GlfwWindow::SetCursorVisible(bool visible)
	{
		YUICY_PROFILE_FUNCTION();

		glfwSetInputMode(m_Window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
	}

	void GlfwWindow::Close()
	{
		glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
		WindowCloseEvent event;
		m_Data.EventCallback(event);
	}

	void GlfwWindow::CancelClose()
	{
		glfwSetWindowShouldClose(m_Window, GLFW_FALSE);
	}

	void GlfwWindow::Minimize()
	{
		glfwIconifyWindow(m_Window);
	}

	void GlfwWindow::Maximize()
	{
		glfwMaximizeWindow(m_Window);
	}

	void GlfwWindow::Restore()
	{
		glfwRestoreWindow(m_Window);
	}

	bool GlfwWindow::IsMaximized() const
	{
		return glfwGetWindowAttrib(m_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
	}

}
