#pragma once

#ifdef PLATFORM_WINDOWS
#ifdef YUICY_DYNAMIC_LINK
	#ifdef YUICY_EXPORT_DLL
		#define YUICY_API __declspec(dllexport)
	#else
		#define YUICY_API __declspec(dllimport)
	#endif
#else
	#define YUICY_API
#endif
#else
	#error Yuicy Only Support Windows! 
#endif
