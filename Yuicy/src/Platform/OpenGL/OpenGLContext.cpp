#include "pch.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Platform/OpenGL/OpenGLDebug.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace Yuicy {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: _windowHandle(windowHandle)
	{
		YUICY_ASSERT(windowHandle, "Window handle is null!")
	}

	void OpenGLContext::SetWindowHints()
	{
		// 4.1 Core 是 macOS 支持的最高版本，所有平台统一以它为基线
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		// macOS 不设置 forward compat 只能拿到 2.1 上下文；其他平台设置后会禁用已废弃的功能（如宽度大于 1 的线段）
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#ifndef NDEBUG
		// 驱动支持 GL 4.3 时才会产生调试输出，macOS 上无效果
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
	}

	void OpenGLContext::Init()
	{
		YUICY_PROFILE_FUNCTION();

		glfwMakeContextCurrent(_windowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		YUICY_ASSERT(status, "Failed to initialize Glad!");

		YUICY_CORE_INFO("OpenGL Info:");
		YUICY_CORE_INFO("  Vendor: {}", (const char*)glGetString(GL_VENDOR));
		YUICY_CORE_INFO("  Renderer: {}", (const char*)glGetString(GL_RENDERER));
		YUICY_CORE_INFO("  Version: {}", (const char*)glGetString(GL_VERSION));

		YUICY_CORE_ASSERT(GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 1), "Yuicy requires at least OpenGL version 4.1!");

		OpenGLEnableDebugOutput();
	}

	void OpenGLContext::SwapBuffers()
	{
		YUICY_PROFILE_FUNCTION();

		// 兜底：捕获本帧中没有被就近检查到的 GL 错误
		OpenGLCheckErrors("frame");
		glfwSwapBuffers(_windowHandle);
	}

}
