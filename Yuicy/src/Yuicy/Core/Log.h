#pragma once

#include "Yuicy/Core/Core.h"

#ifdef REFLECTION_STRUCT
	#include "tinyrefl/reflection_to_json.hpp"
	#include "tinyrefl/reflection_from_json.hpp"
#endif

#include "Yuicy/Core/Base.h"
#include <ostream>
#pragma warning(push, 0)
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

// Core log macros
#define YUICY_CORE_TRACE(...)    ::Yuicy::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define YUICY_CORE_INFO(...)     ::Yuicy::Log::GetCoreLogger()->info(__VA_ARGS__)
#define YUICY_CORE_WARN(...)     ::Yuicy::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define YUICY_CORE_ERROR(...)    ::Yuicy::Log::GetCoreLogger()->error(__VA_ARGS__)
#define YUICY_CORE_CRITICAL(...) ::Yuicy::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define YUICY_TRACE(...)         ::Yuicy::Log::GetClientLogger()->trace(__VA_ARGS__)
#define YUICY_INFO(...)          ::Yuicy::Log::GetClientLogger()->info(__VA_ARGS__)
#define YUICY_WARN(...)          ::Yuicy::Log::GetClientLogger()->warn(__VA_ARGS__)
#define YUICY_ERROR(...)         ::Yuicy::Log::GetClientLogger()->error(__VA_ARGS__)
#define YUICY_CRITICAL(...)      ::Yuicy::Log::GetClientLogger()->critical(__VA_ARGS__)


