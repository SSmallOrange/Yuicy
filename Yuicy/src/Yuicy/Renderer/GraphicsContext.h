#pragma once

namespace Yuicy {

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void SwapBuffers() = 0;

		// 必须在创建原生窗口之前调用：由当前后端设置窗口创建参数（如 OpenGL 的版本与 profile），窗口创建后无法更改
		static void SetWindowHints();
		static Scope<GraphicsContext> Create(void* window);
	};

}