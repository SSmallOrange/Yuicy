#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include "Yuicy/Core/Log.h"
#include "Yuicy/Project/Project.h"

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

}

int main(int argc, char** argv)
{
	doctest::Context context;
	context.applyCommandLine(argc, argv);

	// 必须先于 context.run()：logger 未初始化时 YUICY_CORE_* 会解引用空指针
	InitTestLogging();

	const int result = context.run();

	// 兜底关闭用例直接设置、未自行关闭的活动项目：留到静态析构阶段，EditorAssetManager 写注册表时可能用到已销毁的 logger
	Yuicy::Project::SetActive(nullptr);

	return result;
}
