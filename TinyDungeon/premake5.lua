project "TinyDungeon"
    location "."
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir    ("%{wks.location}/bin/int/" .. outputdir .. "/%{prj.name}")
    
    files { 
        "src/**.h", 
        "src/**.hpp", 
        "src/**.cpp",
        "assets/**.*"
    }
    
    includedirs { 
        "%{wks.location}/Yuicy/src", 
        "%{wks.location}/Yuicy/thirdparty/spdlog/include", 
        "%{wks.location}/Yuicy/thirdparty/GLFW/include",
        "%{wks.location}/Yuicy/thirdparty",
        "%{wks.location}/Yuicy/thirdparty/tinyrefl",
        "%{wks.location}/Yuicy/thirdparty/GLAD/include",
        "%{wks.location}/Yuicy/thirdparty/glm",
        "%{wks.location}/Yuicy/thirdparty/entt/include",
        "%{wks.location}/Yuicy/thirdparty/Box2D/box2d/include",
        "%{wks.location}/Yuicy/thirdparty/sol2/include",
        "%{wks.location}/Yuicy/thirdparty/lua/src",
        "%{prj.location}/thirdparty"
    }
    
    links { "Yuicy" }
    defines { "PLATFORM_WINDOWS" }
    
    filter "system:windows"
        systemversion "latest"
        buildoptions { "/utf-8" }
    
    filter "configurations:Debug"
        debugdir "%{cfg.targetdir}"
        runtime "Debug"
        symbols "On"
        defines { "YUICY_PROFILE_DEBUG" }
    
    filter "configurations:Release"
        runtime "Release"
        optimize "On"
    
    filter {}