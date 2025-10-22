#include "pch.h"
#include "Platform/OpenGL/OpenGLContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Yuicy {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: _windowHandle(windowHandle)
	{
		YUICY_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::Init()
	{
		// YUICY_PROFILE_FUNCTION();

		glfwMakeContextCurrent(_windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		YUICY_ASSERT(status, "Failed to initialize Glad!");

		YUICY_CORE_INFO("OpenGL Info:");
		YUICY_CORE_INFO("  Vendor: {}", (const char*)glGetString(GL_VENDOR));
		YUICY_CORE_INFO("  Renderer: {}", (const char*)glGetString(GL_RENDERER));
		YUICY_CORE_INFO("  Version: {}", (const char*)glGetString(GL_VERSION));

		YUICY_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 5), "Yuicy requires at least OpenGL version 4.5!");
	}

	void OpenGLContext::SwapBuffers()
	{
		// YUICY_PROFILE_FUNCTION();

		glfwSwapBuffers(_windowHandle);
	}

}
