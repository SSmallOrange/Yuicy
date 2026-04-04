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

		// 加载 Lua 脚本，不创建实例
		static bool LoadScript(const std::string& filepath);
		// 复制Lua脚本实例
		static sol::table CreateScriptInstance(const std::string& filepath);
		// 清理 Lua 脚本缓存
		static void ClearScriptCache();

	private:
		static void RegisterBindings();

	private:
		static sol::state* s_luaState;
		static std::unordered_map<std::string, sol::load_result> s_scriptCache;
		static bool s_initialized;
	};

}
