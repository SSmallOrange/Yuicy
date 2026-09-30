#include "YuicyTest/TempDirectory.h"

#include "Yuicy/Core/UUID.h"

#include <doctest/doctest.h>

#include <fstream>
#include <string>
#include <system_error>

namespace Yuicy::Test {

	ScopedTempDirectory::ScopedTempDirectory()
	{
		m_Path = std::filesystem::temp_directory_path() / "YuicyTests" / std::to_string((uint64_t)UUID());
		std::filesystem::create_directories(m_Path);
	}

	ScopedTempDirectory::~ScopedTempDirectory()
	{
		// 析构中不能抛异常；删除失败只会在系统临时目录里留下垃圾，不影响用例结果
		std::error_code error;
		std::filesystem::remove_all(m_Path, error);
	}

	std::filesystem::path ScopedTempDirectory::WriteFile(const std::filesystem::path& relativePath, std::string_view content) const
	{
		const std::filesystem::path filePath = m_Path / relativePath;
		std::filesystem::create_directories(filePath.parent_path());

		std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
		REQUIRE_MESSAGE(stream.is_open(), "Failed to create test file: " << filePath.string());
		stream << content;
		return filePath;
	}

}
