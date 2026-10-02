# Yuicy 自有工程（Yuicy / YuiStudio / Sandbox）共用的 CMake 辅助函数

# 递归收集 <dir> 下的源文件。新增 / 删除文件后，CONFIGURE_DEPENDS 会在下次构建时自动重新配置。
function(yuicy_collect_sources out_var dir)
	file(GLOB_RECURSE sources CONFIGURE_DEPENDS
		"${dir}/*.h"
		"${dir}/*.hpp"
		"${dir}/*.cpp"
		"${dir}/*.mm")

	# Platform/<OS>/ 只在对应系统上编译；新增平台目录时在这里补充排除规则
	if(NOT WIN32)
		list(FILTER sources EXCLUDE REGEX "/Platform/Windows/")
	endif()
	if(NOT APPLE)
		list(FILTER sources EXCLUDE REGEX "/Platform/MacOS/")
		list(FILTER sources EXCLUDE REGEX "\\.mm$")
	endif()

	set(${out_var} ${sources} PARENT_SCOPE)
endfunction()

# 自有工程共用的编译设置
function(yuicy_configure_target target)
	target_compile_definitions(${target} PRIVATE
		$<$<CONFIG:Debug>:YUICY_PROFILE_DEBUG>)

	# 不开 unused-parameter：虚函数的默认空实现、GLFW / GL 回调的参数由签名决定，逐个标注收益很低（与 LLVM 的做法一致）
	if(MSVC)
		# TODO: 清零 MSVC /W4 警告后同样启用 /WX（Windows 回归时）
		target_compile_options(${target} PRIVATE /utf-8 /W4 /wd4100)
	else()
		target_compile_options(${target} PRIVATE -Wall -Wextra -Wno-unused-parameter)
		if(YUICY_WARNINGS_AS_ERRORS)
			target_compile_options(${target} PRIVATE -Werror)
		endif()
	endif()

	if(MSVC)
		if(CMAKE_GENERATOR MATCHES "Visual Studio")
			target_compile_options(${target} PRIVATE /MP)
		endif()
	endif()

	# Cocoa 对象交给 ARC 管理
	target_compile_options(${target} PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)

	get_target_property(target_sources ${target} SOURCES)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${target_sources})

	# CMake 会为每种语言各生成一份 PCH；.mm 不走 PCH，避免多编一份 ObjC++ 版本（文件首行仍 include "pch.h"）
	set(objcxx_sources ${target_sources})
	list(FILTER objcxx_sources INCLUDE REGEX "\\.mm$")
	if(objcxx_sources)
		set_source_files_properties(${objcxx_sources} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
	endif()
endfunction()

# 可执行工程：设置 IDE 调试工作目录，并提供 `run-<target>` 目标。
# 资源以 assets/... 相对路径加载，因此工作目录是工程目录（YuiStudio/、Sandbox/）。
function(yuicy_configure_app target)
	set_target_properties(${target} PROPERTIES
		VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

	add_custom_target(run-${target}
		COMMAND $<TARGET_FILE:${target}>
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		DEPENDS ${target}
		USES_TERMINAL)
	set_target_properties(run-${target} PROPERTIES FOLDER "Run")
endfunction()
