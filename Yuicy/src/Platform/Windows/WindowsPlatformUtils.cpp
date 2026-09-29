#include "pch.h"
#include "Yuicy/Core/PlatformUtils.h"

#include <Windows.h>
#include <shellapi.h>

namespace Yuicy {

	// ShellExecuteW 返回值大于 32 表示成功，否则是错误码
	static bool ShellOpen(const std::filesystem::path& path)
	{
		HINSTANCE result = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		return reinterpret_cast<INT_PTR>(result) > 32;
	}

	bool PlatformUtils::RevealInFileBrowser(const std::filesystem::path& path)
	{
		// TODO: 传入文件时改用 SHOpenFolderAndSelectItems 打开所在目录并选中，目前会用默认程序打开文件
		return ShellOpen(path);
	}

	bool PlatformUtils::OpenWithDefaultApp(const std::filesystem::path& path)
	{
		return ShellOpen(path);
	}

}
