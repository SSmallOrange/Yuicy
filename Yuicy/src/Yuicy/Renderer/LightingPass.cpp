#include "pch.h"
#include "LightingPass.h"
#include "Yuicy/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLLightingPass.h"

namespace Yuicy {

	Ref<LightingPass> LightingPass::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:
			YUICY_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
			return nullptr;

		case RendererAPI::API::OpenGL:
			return CreateRef<OpenGLLightingPass>();
		}

		YUICY_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
