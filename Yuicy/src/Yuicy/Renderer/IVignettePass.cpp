#include "pch.h"
#include "Yuicy/Renderer/IVignettePass.h"
#include "Yuicy/Renderer/Renderer.h"
#include "Yuicy/Renderer/RendererAPI.h"

#include "Platform/OpenGL/PostProcess/OpenGLVignettePass.h"

namespace Yuicy {

	Ref<IVignettePass> IVignettePass::Create()
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    return nullptr;
		case RendererAPI::API::OpenGL:  return CreateRef<OpenGLVignettePass>();
		}

		return nullptr;
	}

}
