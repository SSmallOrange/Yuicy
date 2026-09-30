yuicy_collect_sources(YUISTUDIO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src")

# 编辑器逻辑编译为静态库，入口单独成可执行文件：测试工程（tests/）可以直接链接编辑器代码
set(YUISTUDIO_ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/YuiStudioApp.cpp")
list(REMOVE_ITEM YUISTUDIO_SOURCES "${YUISTUDIO_ENTRY_SOURCE}")

add_library(YuiStudioCore STATIC ${YUISTUDIO_SOURCES})
target_precompile_headers(YuiStudioCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")
target_include_directories(YuiStudioCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(YuiStudioCore PUBLIC Yuicy)
yuicy_configure_target(YuiStudioCore)

add_executable(YuiStudio "${YUISTUDIO_ENTRY_SOURCE}")
target_link_libraries(YuiStudio PRIVATE YuiStudioCore)
yuicy_configure_target(YuiStudio)
yuicy_configure_app(YuiStudio)

if(YUICY_BUILD_TESTS)
	add_subdirectory(tests)
endif()
