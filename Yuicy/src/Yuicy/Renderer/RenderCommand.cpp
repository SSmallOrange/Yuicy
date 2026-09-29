#include "pch.h"
#include "Yuicy/Renderer/RenderCommand.h"

namespace Yuicy {

	// 静态初始化期间创建：依赖的 RendererAPI::s_API 是常量初始化，不存在跨翻译单元的初始化顺序问题
	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();
}
