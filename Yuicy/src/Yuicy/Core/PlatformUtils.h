#pragma once

#include <filesystem>

namespace Yuicy {

	// 系统 Shell 集成：交给系统文件管理器或默认程序处理，调用后立即返回，不等待对方完成；
	// 返回 false 表示请求未能发出（路径不存在、没有关联程序等）
	class PlatformUtils
	{
	public:
		// 目录：在文件管理器中打开该目录；文件：打开其所在目录并选中它
		static bool RevealInFileBrowser(const std::filesystem::path& path);
		// 目录会在文件管理器中打开
		static bool OpenWithDefaultApp(const std::filesystem::path& path);
	};

}
