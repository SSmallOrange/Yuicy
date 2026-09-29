#pragma once

// 由编译器预定义宏推导 PLATFORM_*，不依赖构建系统传入。
// Windows 上 CMake 目前也会定义 PLATFORM_WINDOWS，因此每个宏先判断是否已定义。
#if defined(_WIN32)
	#ifndef PLATFORM_WINDOWS
		#define PLATFORM_WINDOWS
	#endif
#elif defined(__APPLE__) && defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_OS_OSX
		#ifndef PLATFORM_MACOS
			#define PLATFORM_MACOS
		#endif
	#else
		#error Yuicy only supports macOS among Apple platforms!
	#endif
#elif defined(__linux__)
	#ifndef PLATFORM_LINUX
		#define PLATFORM_LINUX
	#endif
#else
	#error Unknown platform!
#endif
