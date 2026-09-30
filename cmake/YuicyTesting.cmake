# 单元测试工程的 CMake 辅助函数，结构参考 O3DE（Xxx.Static + Xxx.Tests、ly_add_googletest）：
# - 被测模块编译为静态库（Yuicy、YuiStudioCore），测试可执行文件链接它与 YuicyTestFramework（TestFramework/）；
# - 每个模块在自己目录下放 tests/，产出一个 <Module>Tests（YuicyTests、YuiStudioTests）；
# - 每个 doctest TEST_CASE 注册为一条 CTest 测试，名称为 "<target>.<用例名>"，所在 TEST_SUITE 作为 CTest 标签。

if(NOT YUICY_BUILD_TESTS)
	return()
endif()

include("${YUICY_THIRDPARTY_DIR}/doctest/scripts/cmake/doctest.cmake")

# 单条用例的超时（秒）。单元测试不应接近这个值，超时基本意味着死循环或等待了不存在的窗口事件
set(YUICY_TEST_TIMEOUT 60 CACHE STRING "单条 CTest 用例的超时时间（秒）")

# yuicy_add_test(<target>
#     SOURCE_DIR <dir>        递归收集 <dir> 下的源文件；存在 <dir>/pch.h 时作为预编译头
#     LINK <targets...>       被测的库（Yuicy、YuiStudioCore 等）
#     [DATA_DIR <dir>])       只读测试数据目录，以宏 YUICY_TEST_DATA_DIR（绝对路径字符串）传给测试代码
#
# 用例发现在构建后运行测试程序的 --list-test-cases 完成（doctest_discover_tests），
# 因此测试程序在静态初始化与 main 中都不能依赖窗口、GL 上下文或工作目录下的资源。
function(yuicy_add_test target)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "SOURCE_DIR;DATA_DIR" "LINK")
	if(NOT arg_SOURCE_DIR)
		message(FATAL_ERROR "yuicy_add_test(${target})：缺少 SOURCE_DIR")
	endif()

	yuicy_collect_sources(test_sources "${arg_SOURCE_DIR}")
	add_executable(${target} ${test_sources})

	target_include_directories(${target} PRIVATE "${arg_SOURCE_DIR}")
	target_link_libraries(${target} PRIVATE YuicyTestFramework ${arg_LINK})
	if(EXISTS "${arg_SOURCE_DIR}/pch.h")
		target_precompile_headers(${target} PRIVATE "${arg_SOURCE_DIR}/pch.h")
	endif()
	if(arg_DATA_DIR)
		target_compile_definitions(${target} PRIVATE YUICY_TEST_DATA_DIR="${arg_DATA_DIR}")
	endif()

	yuicy_configure_target(${target})

	# 工作目录放在构建目录：测试运行时产生的 Yuicy.log 等文件不会写进源码树
	set(test_working_dir "${CMAKE_CURRENT_BINARY_DIR}")
	set_target_properties(${target} PROPERTIES
		FOLDER "Tests"
		VS_DEBUGGER_WORKING_DIRECTORY "${test_working_dir}")

	doctest_discover_tests(${target}
		TEST_PREFIX "${target}."
		WORKING_DIRECTORY "${test_working_dir}"
		ADD_LABELS 1
		PROPERTIES TIMEOUT ${YUICY_TEST_TIMEOUT})
endfunction()
