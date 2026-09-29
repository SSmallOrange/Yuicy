#include "pch.h"
#include "Yuicy/Renderer/GraphicsContext.h"

#include "Yuicy/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace Yuicy {

	void GraphicsContext::SetWindowHints()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			YUICY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return;

		case RendererAPI::API::OpenGL:
			OpenGLContext::SetWindowHints();
			return;
		}

		YUICY_CORE_ASSERT(false, "Unknown RendererAPI!");
	}

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			YUICY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
		}

		YUICY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
