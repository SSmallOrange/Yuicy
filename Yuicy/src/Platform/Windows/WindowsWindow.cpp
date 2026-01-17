#include "pch.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Events/KeyEvent.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>

namespace Yuicy {
	
	static uint8_t s_GLFWWindowCount = 0;

	static void GLFWErrorCallback(int error, const char* description)
	{
		YUICY_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		YUICY_PROFILE_FUNCTION();

		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		YUICY_PROFILE_FUNCTION();

		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		YUICY_PROFILE_FUNCTION();

		_Data.Title = props.Title;
		_Data.Width = props.Width;
		_Data.Height = props.Height;

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
			// 无边框窗口
			glfwWindowHint(GLFW_DECORATED, props.borderlessWindow ? GLFW_FALSE : GLFW_TRUE);
			// 创建窗口(提供渲染目标)，创建一个OpenGL上下文
			_Window = glfwCreateWindow((int)props.Width, (int)props.Height, _Data.Title.c_str(), nullptr, nullptr);
			++s_GLFWWindowCount;
		}

		 _Context = GraphicsContext::Create(_Window);
		 _Context->Init();

		glfwSetWindowUserPointer(_Window, &_Data);
		SetVSync(true);

		// Set GLFW callbacks
		glfwSetWindowSizeCallback(_Window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallback(event);
		});

		glfwSetWindowCloseCallback(_Window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallback(event);
		});

		glfwSetKeyCallback(_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
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

		glfwSetCharCallback(_Window, [](GLFWwindow* window, unsigned int keycode)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			KeyTypedEvent event(keycode);
			data.EventCallback(event);
		});

		glfwSetMouseButtonCallback(_Window, [](GLFWwindow* window, int button, int action, int mods)
		{
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

		glfwSetScrollCallback(_Window, [](GLFWwindow* window, double xOffset, double yOffset)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallback(event);
		});

		glfwSetCursorPosCallback(_Window, [](GLFWwindow* window, double xPos, double yPos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallback(event);
		});
	}

	void WindowsWindow::Shutdown()
	{
		YUICY_PROFILE_FUNCTION();

		if (m_customCursor)
		{
			glfwDestroyCursor(m_customCursor);
			m_customCursor = nullptr;
		}

		glfwDestroyWindow(_Window);
		--s_GLFWWindowCount;

		if (s_GLFWWindowCount == 0)
		{
			glfwTerminate();
		}
	}

	void WindowsWindow::OnUpdate()
	{
		YUICY_PROFILE_FUNCTION();

		glfwPollEvents();
		_Context->SwapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		YUICY_PROFILE_FUNCTION();

		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return _Data.VSync;
	}

	void WindowsWindow::SetCursor(const std::string& imagePath, int hotspotX, int hotspotY)
	{
		YUICY_PROFILE_FUNCTION();

		if (m_customCursor)
		{
			glfwDestroyCursor(m_customCursor);
			m_customCursor = nullptr;
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

		m_customCursor = glfwCreateCursor(&image, hotspotX, hotspotY);
		stbi_image_free(pixels);

		if (m_customCursor)
		{
			glfwSetCursor(_Window, m_customCursor);
			YUICY_CORE_INFO("Custom cursor set: {0}", imagePath);
		}
		else
		{
			YUICY_CORE_ERROR("Failed to create cursor from image: {0}", imagePath);
		}
	}

	void WindowsWindow::ResetCursor()
	{
		YUICY_PROFILE_FUNCTION();

		if (m_customCursor)
		{
			glfwDestroyCursor(m_customCursor);
			m_customCursor = nullptr;
		}

		glfwSetCursor(_Window, nullptr);
	}

	void WindowsWindow::SetCursorVisible(bool visible)
	{
		YUICY_PROFILE_FUNCTION();

		if (visible)
			glfwSetInputMode(_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		else
			glfwSetInputMode(_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	void WindowsWindow::Close()
	{
		glfwSetWindowShouldClose(_Window, GLFW_TRUE);
		WindowCloseEvent event;
		_Data.EventCallback(event);
	}

	void WindowsWindow::Minimize()
	{
		glfwIconifyWindow(_Window);
	}

	void WindowsWindow::Maximize()
	{
		glfwMaximizeWindow(_Window);
	}

	void WindowsWindow::Restore()
	{
		glfwRestoreWindow(_Window);
	}

	bool WindowsWindow::IsMaximized() const
	{
		return glfwGetWindowAttrib(_Window, GLFW_MAXIMIZED) == GLFW_TRUE;
	}

}
