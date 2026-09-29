yuicy_collect_sources(YUICY_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(Yuicy STATIC
	${YUICY_SOURCES}
	"${YUICY_THIRDPARTY_DIR}/stb_image/stb_image.h"
	"${YUICY_THIRDPARTY_DIR}/stb_image/stb_image.cpp")

target_precompile_headers(Yuicy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h")

target_include_directories(Yuicy PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
# 客户端以 "imgui/imgui.h" 的形式引用第三方头文件
target_include_directories(Yuicy SYSTEM INTERFACE "${YUICY_THIRDPARTY_DIR}")

target_compile_definitions(Yuicy PRIVATE
	YUICY_EXPORT_DLL
	YUICY_ENABLE_ASSERTS
	GLFW_INCLUDE_NONE
	$<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

# 客户端会 include 引擎头文件（及其引用的第三方头文件），因此依赖以 PUBLIC 传递
target_link_libraries(Yuicy PUBLIC
	glfw
	glad
	imgui
	box2d
	lua
	yaml-cpp
	yuicy_header_deps)

# 平台宏只在 Windows 上定义（原 premake 在任何系统上都定义它）；PUBLIC 传递给 YuiStudio / Sandbox
if(WIN32)
	target_compile_definitions(Yuicy PUBLIC PLATFORM_WINDOWS)
	target_link_libraries(Yuicy PUBLIC opengl32 user32 gdi32 shell32)
endif()

yuicy_configure_target(Yuicy)
