#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Log.h"
#include <filesystem>

#ifdef YUICY_ENABLE_ASSERTS

#if defined(_MSC_VER)
	#define YUICY_DEBUGBREAK() __debugbreak()
#elif defined(__GNUC__)
	#include <signal.h>
	#define YUICY_DEBUGBREAK() raise(SIGTRAP)
#else
	#define YUICY_DEBUGBREAK() ((void)0)
#endif
	// 提供两种断言：YUICY_ASSERT(ptr, "Pointer is null"); 和 YUICY_ASSERT(x > 0);
	#define YUICY_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { YUICY##type##ERROR(msg, __VA_ARGS__); YUICY_DEBUGBREAK(); } }
	#define YUICY_INTERNAL_ASSERT_WITH_MSG(type, check, ...) YUICY_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define YUICY_INTERNAL_ASSERT_NO_MSG(type, check) YUICY_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", YUICY_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define YUICY_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define YUICY_INTERNAL_ASSERT_GET_MACRO(...) YUICY_EXPAND_MACRO( YUICY_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, YUICY_INTERNAL_ASSERT_WITH_MSG, YUICY_INTERNAL_ASSERT_NO_MSG) )

	// 至多支持两个参数，不支持：YUICY_ASSERT(a < b, "a={}, b={}", a, b);
	#define YUICY_ASSERT(...) YUICY_EXPAND_MACRO( YUICY_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define YUICY_CORE_ASSERT(...) YUICY_EXPAND_MACRO( YUICY_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define YUICY_ASSERT(...)
	#define YUICY_CORE_ASSERT(...)
#endif
