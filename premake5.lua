workspace "Yuicy"
    architecture "x64"
    startproject "Sandbox"
    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-x64"
rundir    = "%{wks.location}bin\\" .. outputdir .. "\\Sandbox"

group "Dependencies"
    include "Yuicy/thirdparty/GLFW"
    include "Yuicy/thirdparty/GLAD"
    include "Yuicy/thirdparty/imgui"

group ""
project "Yuicy"
    location "Yuicy"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin/int/" .. outputdir .. "/%{prj.name}")
    pchheader "pch.h"
    pchsource "Yuicy/src/pch.cpp"
    files {
        "Yuicy/src/**.h",
        "Yuicy/src/**.hpp",
        "Yuicy/src/**.cpp", 
        "Yuicy/thirdparty/glm/glm/**.hpp",
        "Yuicy/thirdparty/glm/glm/**.inl"
    }
    includedirs { 
        "Yuicy/src", 
        "Yuicy/thirdparty/spdlog/include", 
        "Yuicy/thirdparty/GLFW/include" , 
        "Yuicy/thirdparty/tinyrefl", 
        "Yuicy/thirdparty/GLAD/include",
        "Yuicy/thirdparty/imgui",
        "Yuicy/thirdparty/glm"
    }
    defines { 
        "PLATFORM_WINDOWS",
        "YUICY_EXPORT_DLL", 
        "YUICY_ENABLE_ASSERTS",
        "GLFW_INCLUDE_NONE",         -- GLFW不包含OpenGL头文件
        "_CRT_SECURE_NO_WARNINGS"
    }
	links { 
        "GLFW", 
        "Glad",
        "imgui"
    }
    filter "system:windows"
        systemversion "latest"
		links { "opengl32", "user32", "gdi32", "shell32" }
        buildoptions { "/utf-8" }
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "YUICY_PROFILE_DEBUG" }
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
        defines { "NDEBUG" }    -- spdlog
    filter {}

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin/int/" .. outputdir .. "/%{prj.name}")
    files { "Sandbox/src/**.h", "Sandbox/src/**.hpp", "Sandbox/src/**.cpp" }
    includedirs { 
        "Yuicy/src", 
        "Yuicy/thirdparty/spdlog/include", 
        "Yuicy/thirdparty/GLFW/include" , 
        "Yuicy/thirdparty/tinyrefl", 
        "Yuicy/thirdparty/GLAD/include",
        "Yuicy/thirdparty/glm"
    }
    links { "Yuicy" }
    defines { "PLATFORM_WINDOWS" }
    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
        postbuildcommands {
			'cmd /c if not exist "%{cfg.targetdir}" mkdir "%{cfg.targetdir}"',
			-- library
			'cmd /c if exist "%{wks.location}bin\\%{cfg.buildcfg}-x64\\Yuicy\\*.dll" copy /Y "%{wks.location}bin\\%{cfg.buildcfg}-x64\\Yuicy\\*.dll" "%{cfg.targetdir}"',
			-- 'cmd /c if exist "%{wks.location}Yuicy\\thirdparty\\GLFW\\bin\\%{cfg.buildcfg}-x64\\GLFW\\*.lib" copy /Y "%{wks.location}Yuicy\\thirdparty\\GLFW\\bin\\%{cfg.buildcfg}-x64\\GLFW\\*.lib" "%{cfg.targetdir}"',
			-- pdb
			'cmd /c if exist "%{wks.location}bin\\%{cfg.buildcfg}-x64\\Yuicy\\*.pdb" copy /Y "%{wks.location}bin\\%{cfg.buildcfg}-x64\\Yuicy\\*.pdb" "%{cfg.targetdir}"',
			-- 'cmd /c if exist "%{wks.location}Yuicy\\thirdparty\\GLFW\\bin\\%{cfg.buildcfg}-x64\\GLFW\\*.pdb" copy /Y "%{wks.location}Yuicy\\thirdparty\\GLFW\\bin\\%{cfg.buildcfg}-x64\\GLFW\\*.pdb" "%{cfg.targetdir}"'
        }
    filter "configurations:Debug"
        debugdir "%{cfg.targetdir}"
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
    filter {}
