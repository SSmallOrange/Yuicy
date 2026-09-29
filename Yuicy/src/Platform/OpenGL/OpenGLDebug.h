#pragma once

namespace Yuicy {

	// 注册 glDebugMessageCallback。需要 GL 4.3 与 debug 上下文（OpenGLContext::SetWindowHints），
	// 不满足时（如 macOS 最高 4.1）什么也不做；Release 下为空操作
	void OpenGLEnableDebugOutput();

	// 取出并记录所有累积的 glGetError 错误，location 用于标明检查点；Release 下为空操作。
	// macOS 上没有调试输出，这是主要的排查手段，应在资源创建等关键位置调用
	void OpenGLCheckErrors(const char* location);

}
