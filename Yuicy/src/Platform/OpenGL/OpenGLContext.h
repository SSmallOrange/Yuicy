#pragma once

#include "Yuicy/Renderer/GraphicsContext.h"

struct GLFWwindow;

namespace Yuicy {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
	private:
		GLFWwindow* _windowHandle;
	};

}