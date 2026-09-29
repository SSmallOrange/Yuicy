#include "pch.h"
#include "Yuicy/Core/Window.h"

#if defined(PLATFORM_WINDOWS)
	#include "Platform/Windows/WindowsWindow.h"
#elif defined(PLATFORM_MACOS)
	#include "Platform/GLFW/GlfwWindow.h"
#endif

namespace Yuicy
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#if defined(PLATFORM_WINDOWS)
		return CreateScope<WindowsWindow>(props);
	#elif defined(PLATFORM_MACOS)
		return CreateScope<GlfwWindow>(props);
	#else
		YUICY_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}

}