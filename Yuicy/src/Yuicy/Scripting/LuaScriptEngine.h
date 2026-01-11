#pragma once

#include <string>
#include <unordered_map>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

namespace Yuicy {

	class LuaScriptEngine
	{
	public:
		LuaScriptEngine() = default;
		~LuaScriptEngine();

	public:
		static void Init();
		static void Shutdown();

		static sol::state& GetState() { return *s_luaState; }

		static bool LoadScript(const std::string& filepath);
		static sol::table CreateScriptInstance(const std::string& filepath);
		static void ClearScriptCache();

	private:
		static void RegisterBindings();

	private:
		static sol::state* s_luaState;
		static std::unordered_map<std::string, sol::load_result> s_scriptCache;
		static bool s_initialized;
	};

}
