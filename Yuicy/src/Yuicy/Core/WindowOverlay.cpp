#include "pch.h"
#include "Yuicy/Core/WindowOverlay.h"

#ifdef PLATFORM_WINDOWS
	#include "Platform/Windows/WindowsWindowOverlay.h"
#endif

namespace Yuicy {

	Scope<WindowOverlay> WindowOverlay::Create()
	{
#ifdef PLATFORM_WINDOWS
		return CreateScope<WindowsWindowOverlay>();
#else
		YUICY_CORE_ASSERT(false, "Unknown platform!");
		return nullptr;
#endif
	}

}
