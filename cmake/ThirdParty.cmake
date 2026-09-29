# 第三方依赖的 CMake target。
# 规则：不修改 Yuicy/thirdparty/** 下的任何文件；上游自带 CMakeLists 的用 add_subdirectory，
# 其余（GLAD / imgui / lua / 纯头文件库）在这里声明。

set(YUICY_THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/Yuicy/thirdparty")

# 第三方库一律编译为静态库
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ---- GLFW（子模块，自带 CMake） ------------------------------------------------
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF CACHE BOOL "" FORCE)
add_subdirectory("${YUICY_THIRDPARTY_DIR}/GLFW" EXCLUDE_FROM_ALL)

# ---- Box2D 2.4.1（子模块，自带 CMake） -----------------------------------------
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED    OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS       OFF CACHE BOOL "" FORCE)
# 上游 cmake_minimum_required(VERSION 3.8) 会触发 CMake 4 的弃用警告，与本项目无关
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
add_subdirectory("${YUICY_THIRDPARTY_DIR}/Box2D/box2d" EXCLUDE_FROM_ALL)
unset(CMAKE_WARN_DEPRECATED CACHE)

# ---- yaml-cpp（子模块，自带 CMake；PUBLIC 导出 YAML_CPP_STATIC_DEFINE） ------------
set(YAML_CPP_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS   OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL       OFF CACHE BOOL "" FORCE)
set(YAML_CPP_FORMAT_SOURCE OFF CACHE BOOL "" FORCE)
add_subdirectory("${YUICY_THIRDPARTY_DIR}/yaml-cpp" EXCLUDE_FROM_ALL)

# ---- GLAD --------------------------------------------------------------------
add_library(glad STATIC
	"${YUICY_THIRDPARTY_DIR}/GLAD/src/glad.c"
	"${YUICY_THIRDPARTY_DIR}/GLAD/include/glad/glad.h"
	"${YUICY_THIRDPARTY_DIR}/GLAD/include/KHR/khrplatform.h")
target_include_directories(glad PUBLIC "${YUICY_THIRDPARTY_DIR}/GLAD/include")

# ---- Dear ImGui（后端由 Yuicy/src/Yuicy/ImGui/ImGuiBuild.cpp 编译） -----------------
set(IMGUI_DIR "${YUICY_THIRDPARTY_DIR}/imgui")
add_library(imgui STATIC
	"${IMGUI_DIR}/imconfig.h"
	"${IMGUI_DIR}/imgui.h"
	"${IMGUI_DIR}/imgui.cpp"
	"${IMGUI_DIR}/imgui_demo.cpp"
	"${IMGUI_DIR}/imgui_draw.cpp"
	"${IMGUI_DIR}/imgui_internal.h"
	"${IMGUI_DIR}/imgui_tables.cpp"
	"${IMGUI_DIR}/imgui_widgets.cpp"
	"${IMGUI_DIR}/imstb_rectpack.h"
	"${IMGUI_DIR}/imstb_textedit.h"
	"${IMGUI_DIR}/imstb_truetype.h")
target_include_directories(imgui PUBLIC "${IMGUI_DIR}")

# ---- Lua 5.4 -----------------------------------------------------------------
file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
	"${YUICY_THIRDPARTY_DIR}/lua/src/*.c"
	"${YUICY_THIRDPARTY_DIR}/lua/src/*.h")
list(REMOVE_ITEM LUA_SOURCES
	"${YUICY_THIRDPARTY_DIR}/lua/src/lua.c"
	"${YUICY_THIRDPARTY_DIR}/lua/src/luac.c")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "${YUICY_THIRDPARTY_DIR}/lua/src")
if(MSVC)
	target_compile_definitions(lua PRIVATE _CRT_SECURE_NO_WARNINGS)
	target_compile_options(lua PRIVATE /utf-8)
elseif(APPLE)
	# 与 Lua 官方 Makefile 的 `make macosx` 一致（mkstemp、dlopen 等）
	target_compile_definitions(lua PRIVATE LUA_USE_MACOSX)
endif()

# ---- 纯头文件库 -------------------------------------------------------------------
# 只需要 include 路径，不使用上游 CMakeLists（sol2 要求 CMake >= 3.26，且会引入无关目标）
add_library(yuicy_header_deps INTERFACE)
target_include_directories(yuicy_header_deps INTERFACE
	"${YUICY_THIRDPARTY_DIR}/spdlog/include"
	"${YUICY_THIRDPARTY_DIR}/glm"
	"${YUICY_THIRDPARTY_DIR}/entt/include"
	"${YUICY_THIRDPARTY_DIR}/sol2/include"
	"${YUICY_THIRDPARTY_DIR}/tinyrefl"
	"${YUICY_THIRDPARTY_DIR}/stb_image")

# ---- 统一处理 ----------------------------------------------------------------------
set(YUICY_THIRDPARTY_TARGETS glfw box2d yaml-cpp glad imgui lua)
foreach(dep IN LISTS YUICY_THIRDPARTY_TARGETS)
	# 第三方代码的警告与本项目无关，编译时关闭
	target_compile_options(${dep} PRIVATE $<IF:$<C_COMPILER_ID:MSVC>,/w,-w>)
	set_target_properties(${dep} PROPERTIES
		FOLDER "Dependencies"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
endforeach()

# 第三方头文件以 SYSTEM 方式引入，避免其警告出现在引擎代码的编译输出中
# （等价于 CMake 3.25 的 SYSTEM 属性，这里保持 3.21 兼容）
foreach(dep IN LISTS YUICY_THIRDPARTY_TARGETS ITEMS yuicy_header_deps)
	get_target_property(dep_includes ${dep} INTERFACE_INCLUDE_DIRECTORIES)
	if(dep_includes)
		set_property(TARGET ${dep} APPEND PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES ${dep_includes})
	endif()
endforeach()

if(TARGET update_mappings)
	set_target_properties(update_mappings PROPERTIES FOLDER "Dependencies")
endif()
