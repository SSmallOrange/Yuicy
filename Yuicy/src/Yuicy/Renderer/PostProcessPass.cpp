#include "pch.h"

#include "Renderer.h"
#include "PostProcessPass.h"
#include "Platform/OpenGL/OpenGLPostProcessPass.h"

namespace Yuicy {

	Ref<PostProcessPass> PostProcessPass::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    YUICY_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLPostProcessPass>();
		}

		YUICY_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}