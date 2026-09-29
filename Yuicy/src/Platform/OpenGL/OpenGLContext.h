#pragma once

#include "Yuicy/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Yuicy {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		// 必须在 glfwCreateWindow 之前调用：上下文版本与 profile 只能通过窗口 hint 指定，窗口创建后无法更改
		static void SetWindowHints();

		virtual void Init() override;
		virtual void SwapBuffers() override;
	private:
		GLFWwindow* _windowHandle;
	};

}