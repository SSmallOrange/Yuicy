workspace "Yuicy"
    architecture "x64"
    startproject "Sandbox"
    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-x64"
rundir    = "%{wks.location}bin\\" .. outputdir .. "\\Sandbox"

group "Dependencies"
    include "Yuicy/thirdparty/GLFW"
    include "Yuicy/thirdparty/GLAD"

group ""
project "Yuicy"
    location "Yuicy"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin/int/" .. outputdir .. "/%{prj.name}")
    pchheader "pch.h"
    pchsource "Yuicy/src/pch.cpp"
    files { "Yuicy/src/**.h", "Yuicy/src/**.hpp", "Yuicy/src/**.cpp" }
    includedirs { 
        "Yuicy/src", 
        "Yuicy/thirdparty/spdlog/include", 
        "Yuicy/thirdparty/GLFW/include" , 
        "Yuicy/thirdparty/tinyrefl", 
        "Yuicy/thirdparty/GLAD/include"
    }
    defines { 
        "PLATFORM_WINDOWS",
        "YUICY_EXPORT_DLL", 
        "YUICY_ENABLE_ASSERTS",
        "GLFW_INCLUDE_NONE"         -- GLFW不包含OpenGL头文件
    }
	links { "GLFW", "Glad" }
    filter "system:windows"
        systemversion "latest"
		links { "opengl32", "user32", "gdi32", "shell32" }
        buildoptions { "/utf-8" }
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
    filter {}

project "Sandbox"
    location "Sandbox"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "Off"
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("bin/int/" .. outputdir .. "/%{prj.name}")
    files { "Sandbox/src/**.h", "Sandbox/src/**.hpp", "Sandbox/src/**.cpp" }
    includedirs { 
        "Yuicy/src", 
        "Yuicy/thirdparty/spdlog/include", 
        "Yuicy/thirdparty/GLFW/include" , 
        "Yuicy/thirdparty/tinyrefl", 
        "Yuicy/thirdparty/GLAD/include"
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
        runtime "Debug"
        symbols "On"
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
    filter {}
