cmake_minimum_required(VERSION 3.21)

# 以下两项必须在 project() 之前设置
# 静态 CRT（/MT、/MTd），对应原 premake 的 staticruntime "On"；所有第三方库统一使用
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
# 让 cmake_minimum_required 较低的子项目（如 Box2D）也遵守 CMAKE_MSVC_RUNTIME_LIBRARY
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)

project(Yuicy LANGUAGES C CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "请勿在源码目录内构建，使用 `cmake --preset <name>`（见 CMakePresets.json）")
endif()

get_property(YUICY_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(YUICY_MULTI_CONFIG)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type (Debug / Release)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 输出目录：<build>/bin 放可执行文件，<build>/lib 放静态库（多配置生成器会再追加 /<Config>）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(YuicyHelpers)
include(ThirdParty)

add_subdirectory(Yuicy)
add_subdirectory(Sandbox)
add_subdirectory(YuiStudio)

set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT YuiStudio)

# clangd 默认在源码根目录查找 compile_commands.json；单配置生成器（Makefile / Ninja）才会生成它
if(NOT YUICY_MULTI_CONFIG AND NOT CMAKE_HOST_WIN32)
	file(CREATE_LINK
		"${CMAKE_BINARY_DIR}/compile_commands.json"
		"${CMAKE_SOURCE_DIR}/compile_commands.json"
		SYMBOLIC)
endif()
