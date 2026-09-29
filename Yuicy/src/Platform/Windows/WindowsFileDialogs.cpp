#include "pch.h"
#include "Yuicy/Core/FileDialogs.h"

#include "Yuicy/Core/Application.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#include <commdlg.h>

namespace Yuicy {

	// 拼成 OPENFILENAME::lpstrFilter 的格式："名称 (*.a;*.b)\0*.a;*.b\0"，整体再以一个额外的 \0 结尾
	static std::string ToWin32Filter(std::span<const FileDialogFilter> filters)
	{
		std::string result;
		for (const FileDialogFilter& filter : filters)
		{
			std::string pattern;
			for (const std::string& extension : filter.Extensions)
			{
				if (!pattern.empty())
					pattern += ';';
				pattern += "*." + extension;
			}
			if (pattern.empty())
				pattern = "*.*";

			result += filter.Name + " (" + pattern + ")";
			result += '\0';
			result += pattern;
			result += '\0';
		}
		result += '\0';
		return result;
	}

	std::optional<std::filesystem::path> FileDialogs::OpenFile(std::span<const FileDialogFilter> filters)
	{
		std::string filter = ToWin32Filter(filters);

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.c_str();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return std::filesystem::path(ofn.lpstrFile);

		return std::nullopt;
	}

	std::optional<std::filesystem::path> FileDialogs::SaveFile(std::span<const FileDialogFilter> filters)
	{
		std::string filter = ToWin32Filter(filters);

		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter.c_str();
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		// lpstrDefExt 不带点
		if (!filters.empty() && !filters.front().Extensions.empty())
			ofn.lpstrDefExt = filters.front().Extensions.front().c_str();

		if (GetSaveFileNameA(&ofn) == TRUE)
			return std::filesystem::path(ofn.lpstrFile);

		return std::nullopt;
	}

}
