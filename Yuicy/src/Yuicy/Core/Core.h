#pragma once

#ifdef PLATFORM_WINDOWS
	#ifdef YUICY_EXPORT_DLL
		#define YUICY_API __declspec(dllexport)
	#else
		#define YUICY_API __declspec(dllimport)
	#endif
#else
	#error Yuicy Only Support Windows! 
#endif
