#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Yuicy {

	struct FileDialogFilter
	{
		std::string Name;                    // 显示名称，如 "Yuicy Scene"
		std::vector<std::string> Extensions; // 不带点，如 "yui"；为空表示不限制类型
	};

	// 系统原生文件对话框：模态阻塞直到用户关闭，只能在主线程调用；用户取消时返回 std::nullopt
	class FileDialogs
	{
	public:
		static std::optional<std::filesystem::path> OpenFile(std::span<const FileDialogFilter> filters);
		// 用户输入的文件名不带扩展名时，自动补上 filters 中的第一个扩展名
		static std::optional<std::filesystem::path> SaveFile(std::span<const FileDialogFilter> filters);
	};

}
