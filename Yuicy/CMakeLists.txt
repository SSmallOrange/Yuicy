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

# Cocoa 已由 glfw 传递；UTType（文件对话框的类型过滤）在单独的框架里
if(APPLE)
	target_link_libraries(Yuicy PRIVATE "-framework UniformTypeIdentifiers")
endif()

yuicy_configure_target(Yuicy)

# 外部拷贝进来的源码按原样编译，不参与自有代码的警告门禁
set_source_files_properties(
	"${CMAKE_CURRENT_SOURCE_DIR}/src/Yuicy/ImGui/ImGuizmo.cpp"
	"${YUICY_THIRDPARTY_DIR}/stb_image/stb_image.cpp"
	PROPERTIES COMPILE_OPTIONS "$<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>")

if(YUICY_BUILD_TESTS)
	add_subdirectory(tests)
endif()
