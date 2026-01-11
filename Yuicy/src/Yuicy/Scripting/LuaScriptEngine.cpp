#include "pch.h"
#include "LuaScriptEngine.h"
#include "LuaBindings.h"
#include "Yuicy/Core/Log.h"

#include <fstream>
#include <sstream>

namespace Yuicy {

	sol::state* LuaScriptEngine::s_luaState = nullptr;
	std::unordered_map<std::string, sol::load_result> LuaScriptEngine::s_scriptCache;
	bool LuaScriptEngine::s_initialized = false;

	LuaScriptEngine::~LuaScriptEngine()
	{
		delete s_luaState;
	}

	void LuaScriptEngine::Init()
	{
		if (s_initialized)
			return;

		YUICY_CORE_INFO("LuaScriptEngine: Initializing...");
		s_luaState = new sol::state();
		// Open standard Lua libraries
		s_luaState->open_libraries(
			sol::lib::base,
			sol::lib::math,
			sol::lib::string,
			sol::lib::table,
			sol::lib::os,
			sol::lib::io
		);

		RegisterBindings();

		s_initialized = true;
		YUICY_CORE_INFO("LuaScriptEngine: Initialized successfully");
	}

	void LuaScriptEngine::Shutdown()
	{
		if (!s_initialized)
			return;

		YUICY_CORE_INFO("LuaScriptEngine: Shutting down...");
		ClearScriptCache();
		s_initialized = false;
	}

	void LuaScriptEngine::RegisterBindings()
	{
		LuaBindings::RegisterAll(*s_luaState);
	}

	bool LuaScriptEngine::LoadScript(const std::string& filepath)
	{
		if (s_scriptCache.find(filepath) != s_scriptCache.end())
			return true;

		std::ifstream file(filepath);
		if (!file.is_open())
		{
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to open script file: {}", filepath);
			return false;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string scriptContent = buffer.str();
		file.close();

		sol::load_result loadResult = s_luaState->load(scriptContent, filepath);
		if (!loadResult.valid())
		{
			sol::error err = loadResult;
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to load script '{}': {}", filepath, err.what());
			return false;
		}

		s_scriptCache[filepath] = std::move(loadResult);
		YUICY_CORE_TRACE("LuaScriptEngine: Loaded script: {}", filepath);
		return true;
	}

	sol::table LuaScriptEngine::CreateScriptInstance(const std::string& filepath)
	{
		if (!LoadScript(filepath))
			return sol::nil;

		auto it = s_scriptCache.find(filepath);
		if (it == s_scriptCache.end())
			return sol::nil;

		sol::protected_function_result result = it->second();
		if (!result.valid())
		{
			sol::error err = result;
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to execute script '{}': {}", filepath, err.what());
			return sol::nil;
		}

		// 固定脚本返回值
		sol::object obj = result;
		if (!obj.is<sol::table>())
		{
			YUICY_CORE_ERROR("LuaScriptEngine: Script '{}' did not return a table", filepath);
			return sol::nil;
		}

		sol::table classTable = obj.as<sol::table>();
		sol::table instance = s_luaState->create_table();

		for (auto& pair : classTable)
		{
			instance[pair.first] = pair.second;
		}

		return instance;
	}

	void LuaScriptEngine::ClearScriptCache()
	{
		s_scriptCache.clear();
		YUICY_CORE_TRACE("LuaScriptEngine: Script cache cleared");
	}

}
