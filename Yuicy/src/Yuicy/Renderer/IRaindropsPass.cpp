#include "pch.h"
#include "Yuicy/Renderer/IRaindropsPass.h"
#include "Yuicy/Renderer/Renderer.h"
#include "Yuicy/Renderer/RendererAPI.h"

#include "Platform/OpenGL/PostProcess/OpenGLRaindropsPass.h"

namespace Yuicy {

	Ref<IRaindropsPass> IRaindropsPass::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLRaindropsPass>();
		}

		return nullptr;
	}

}
