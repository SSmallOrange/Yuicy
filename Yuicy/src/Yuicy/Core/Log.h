#pragma once

#include "Yuicy/Core/Core.h"

#ifdef REFLECTION_STRUCT
	#include "tinyrefl/reflection_to_json.hpp"
	#include "tinyrefl/reflection_from_json.hpp"
#endif

#include "Yuicy/Core/Base.h"
#include <ostream>
#pragma warning(push, 0)

#ifndef NDEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

namespace Yuicy {
	class YUICY_API Log {
	public:
		using ptr = std::shared_ptr<spdlog::logger>;
		static void Init();

		static ptr& GetCoreLogger() { return s_CoreLogger; }
		static ptr& GetClientLogger() { return s_ClientLogger; }
	private:
		static ptr s_CoreLogger;
		static ptr s_ClientLogger;
	};

}

// template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
// inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector) {
// 	return os << glm::to_string(vector);
// }
// 
// template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
// inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix) {
// 	return os << glm::to_string(matrix);
// }
// 
// template<typename OStream, typename T, glm::qualifier Q>
// inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion) {
// 	return os << glm::to_string(quaternion);
// }

#define YUICY_CORE_TRACE(...)    SPDLOG_LOGGER_CALL(::Yuicy::Log::GetCoreLogger(), spdlog::level::trace, __VA_ARGS__)
#define YUICY_CORE_INFO(...)     SPDLOG_LOGGER_CALL(::Yuicy::Log::GetCoreLogger(), spdlog::level::info,  __VA_ARGS__)
#define YUICY_CORE_WARN(...)     SPDLOG_LOGGER_CALL(::Yuicy::Log::GetCoreLogger(), spdlog::level::warn,  __VA_ARGS__)
#define YUICY_CORE_ERROR(...)    SPDLOG_LOGGER_CALL(::Yuicy::Log::GetCoreLogger(), spdlog::level::err,   __VA_ARGS__)
#define YUICY_CORE_CRITICAL(...) SPDLOG_LOGGER_CALL(::Yuicy::Log::GetCoreLogger(), spdlog::level::critical, __VA_ARGS__)

// Client log macros
#define YUICY_TRACE(...)         SPDLOG_LOGGER_CALL(::Yuicy::Log::GetClientLogger(), spdlog::level::trace, __VA_ARGS__)
#define YUICY_INFO(...)          SPDLOG_LOGGER_CALL(::Yuicy::Log::GetClientLogger(), spdlog::level::info,  __VA_ARGS__)
#define YUICY_WARN(...)          SPDLOG_LOGGER_CALL(::Yuicy::Log::GetClientLogger(), spdlog::level::warn,  __VA_ARGS__)
#define YUICY_ERROR(...)         SPDLOG_LOGGER_CALL(::Yuicy::Log::GetClientLogger(), spdlog::level::err,   __VA_ARGS__)
#define YUICY_CRITICAL(...)      SPDLOG_LOGGER_CALL(::Yuicy::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)


