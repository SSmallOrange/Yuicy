#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "Yuicy/Core/Assert.h"
#include "Yuicy/Core/Log.h"

#include <cstdlib>

namespace {

	// 默认只输出 warn 及以上，避免引擎的 info 日志淹没 doctest 报告；排查问题时设置环境变量 YUICY_TEST_LOG_LEVEL=trace
	void InitTestLogging()
	{
		Yuicy::Log::Init();

		spdlog::level::level_enum level = spdlog::level::warn;
		if (const char* levelName = std::getenv("YUICY_TEST_LOG_LEVEL"))
			level = spdlog::level::from_str(levelName);

		Yuicy::Log::GetCoreLogger()->set_level(level);
		Yuicy::Log::GetClientLogger()->set_level(level);
	}

	// 默认的 YUICY_DEBUGBREAK 在没有调试器时会让进程直接退出，doctest 来不及报告是哪个用例、哪一行。
	// DOCTEST_ADD_FAIL_AT 属于 REQUIRE 级别：记录失败后抛异常中止当前用例，断言之后的代码不会继续执行。
	bool ReportAssertFailureToDoctest(const char* file, int line, const char* expression)
	{
		// 用例之外（如 main 中的全局初始化与清理）没有可以记录失败的上下文，保持默认行为
		if (!doctest::is_running_in_test)
			return false;

		// 先拼好再传入：宏展开为 mb * __VA_ARGS__，参数里的 + / << 会先与 mb 结合
		const std::string message = std::string("YUICY_ASSERT failed: ") + expression;
		DOCTEST_ADD_FAIL_AT(file, line, message);
		return true;
	}
}

int main(int argc, char** argv)
{
	doctest::Context context;
	context.applyCommandLine(argc, argv);

	// 必须先于 context.run()：logger 未初始化时 YUICY_CORE_* 会解引用空指针
	InitTestLogging();
	Yuicy::SetAssertFailureHandler(ReportAssertFailureToDoctest);

	return context.run();
}
