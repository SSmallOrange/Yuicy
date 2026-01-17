#include "pch.h"
#include "Yuicy/Core/Window.h"

#ifdef PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindow.h"
#endif

namespace Yuicy
{
	Scope<Window> Window::Create(const WindowProps& props)
	{
	#ifdef PLATFORM_WINDOWS
		return CreateScope<WindowsWindow>(props);
	#else
		YUICY_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
	#endif
	}

}