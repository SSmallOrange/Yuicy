#include "pch.h"
#include "Platform/OpenGL/OpenGLDebug.h"

#include <glad/glad.h>

namespace Yuicy {

#ifndef NDEBUG
	static const char* GLErrorToString(GLenum error)
	{
		switch (error)
		{
		case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
		case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
		default:                               return "Unknown";
		}
	}

	static void APIENTRY OpenGLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
		GLsizei length, const GLchar* message, const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:   YUICY_CORE_ERROR("[OpenGL] {}", message); return;
		case GL_DEBUG_SEVERITY_MEDIUM: YUICY_CORE_WARN("[OpenGL] {}", message); return;
		default:                       YUICY_CORE_TRACE("[OpenGL] {}", message); return;
		}
	}
#endif

	void OpenGLEnableDebugOutput()
	{
#ifndef NDEBUG
		if (!GLAD_GL_VERSION_4_3)
		{
			YUICY_CORE_INFO("OpenGL debug output unavailable (requires 4.3), falling back to glGetError checks");
			return;
		}

		GLint flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
			return;

		glEnable(GL_DEBUG_OUTPUT);
		// 同步回调：出错时调用栈停在触发错误的 GL 调用上，便于断点定位
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLDebugMessageCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif
	}

	void OpenGLCheckErrors(const char* location)
	{
#ifndef NDEBUG
		// 上下文丢失时部分驱动会持续返回错误，设上限避免死循环
		constexpr int kMaxErrorsPerCheck = 16;
		for (int i = 0; i < kMaxErrorsPerCheck; i++)
		{
			GLenum error = glGetError();
			if (error == GL_NO_ERROR)
				return;

			YUICY_CORE_ERROR("[OpenGL] {} (0x{:04X}) at {}", GLErrorToString(error), error, location);
		}
#endif
	}

}
