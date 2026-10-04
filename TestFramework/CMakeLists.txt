# 各测试工程共用的测试库（对应 O3DE 的 AzTest）：提供 main、日志初始化、临时目录 / 场景夹具，以及 glm 向量的近似比较与打印。
# 只依赖引擎与 doctest，不创建窗口与 GL 上下文。
yuicy_collect_sources(YUICY_TEST_FRAMEWORK_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/YuicyTest")

add_library(YuicyTestFramework STATIC ${YUICY_TEST_FRAMEWORK_SOURCES})
target_include_directories(YuicyTestFramework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(YuicyTestFramework PUBLIC Yuicy doctest)
# MSVC 把标准的 std::getenv 标为不安全（C4996），这里与引擎保持一致关闭该警告，而不是改用 MSVC 专属的 _dupenv_s
target_compile_definitions(YuicyTestFramework PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)
yuicy_configure_target(YuicyTestFramework)
set_target_properties(YuicyTestFramework PROPERTIES FOLDER "Tests")
