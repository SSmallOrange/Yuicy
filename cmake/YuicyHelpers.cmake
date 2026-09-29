# Yuicy 自有工程（Yuicy / YuiStudio / Sandbox）共用的 CMake 辅助函数

# 递归收集 <dir> 下的源文件。新增 / 删除文件后，CONFIGURE_DEPENDS 会在下次构建时自动重新配置。
function(yuicy_collect_sources out_var dir)
	file(GLOB_RECURSE sources CONFIGURE_DEPENDS
		"${dir}/*.h"
		"${dir}/*.hpp"
		"${dir}/*.cpp")
	set(${out_var} ${sources} PARENT_SCOPE)
endfunction()

# 自有工程共用的编译设置
function(yuicy_configure_target target)
	target_compile_definitions(${target} PRIVATE
		$<$<CONFIG:Debug>:YUICY_PROFILE_DEBUG>)

	if(MSVC)
		target_compile_options(${target} PRIVATE /utf-8)
		if(CMAKE_GENERATOR MATCHES "Visual Studio")
			target_compile_options(${target} PRIVATE /MP)
		endif()
	endif()

	get_target_property(target_sources ${target} SOURCES)
	source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${target_sources})
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
