#include "pch.h"
#include "Yuicy/Renderer/ILightingComposePass.h"
#include "Yuicy/Renderer/Renderer.h"
#include "Yuicy/Renderer/RendererAPI.h"

#include "Platform/OpenGL/PostProcess/OpenGLLightingComposePass.h"

namespace Yuicy {

	Ref<ILightingComposePass> ILightingComposePass::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLLightingComposePass>();
		}

		return nullptr;
	}

}
