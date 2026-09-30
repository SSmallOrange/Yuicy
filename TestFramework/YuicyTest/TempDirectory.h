#pragma once

#include <filesystem>
#include <string_view>

namespace Yuicy::Test {

	// 在系统临时目录下创建一个唯一的空目录，析构时连同其中的内容一起删除
	class ScopedTempDirectory
	{
	public:
		ScopedTempDirectory();
		~ScopedTempDirectory();

		ScopedTempDirectory(const ScopedTempDirectory&) = delete;
		ScopedTempDirectory& operator=(const ScopedTempDirectory&) = delete;

		const std::filesystem::path& GetPath() const { return m_Path; }

		// 写入文本文件（按需创建父目录，已存在则覆盖），返回文件的绝对路径；创建失败时终止当前用例
		std::filesystem::path WriteFile(const std::filesystem::path& relativePath, std::string_view content = {}) const;

	private:
		std::filesystem::path m_Path;
	};

}
