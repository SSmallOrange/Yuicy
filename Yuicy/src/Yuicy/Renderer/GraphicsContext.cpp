#include "pch.h"
#include "Yuicy/Renderer/GraphicsContext.h"

// #include "Yuicy/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLContext.h"

namespace Yuicy {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{

		return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));

// 		switch (Renderer::GetAPI())
// 		{
// 			case RendererAPI::API::None:    YUICY_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
// 			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
// 		}

//		YUICY_ASSERT(false, "Unknown RendererAPI!");
//		return nullptr;
	}

}