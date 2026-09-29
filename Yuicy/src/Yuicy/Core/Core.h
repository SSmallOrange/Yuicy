#pragma once

#include "Yuicy/Core/PlatformDetection.h"

#if defined(PLATFORM_WINDOWS) && defined(YUICY_DYNAMIC_LINK)
	#ifdef YUICY_EXPORT_DLL
		#define YUICY_API __declspec(dllexport)
	#else
		#define YUICY_API __declspec(dllimport)
	#endif
#else
	#define YUICY_API
#endif
