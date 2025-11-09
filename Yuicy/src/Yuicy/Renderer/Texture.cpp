#include "pch.h"
#include "Yuicy/Renderer/Texture.h"

#include "Yuicy/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"

namespace Yuicy {

	Ref<Texture2D> Texture2D::Create(const std::string& path)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI::API::None:    YUICY_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::API::OpenGL:  return std::make_shared<OpenGLTexture2D>(path);
		}

		YUICY_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}