#include "pch.h"
#include "Yuicy/Renderer/RenderCommand.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
namespace Yuicy {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;
}