#include "pch.h"
#include "LuaScriptEngine.h"
#include "LuaBindings.h"
#include "Yuicy/Core/Log.h"

#include <fstream>
#include <sstream>
#include <system_error>

namespace Yuicy {

	sol::state* LuaScriptEngine::s_luaState = nullptr;
	std::unordered_map<std::string, LuaScriptEngine::ScriptEntry> LuaScriptEngine::s_scriptCache;
	uint32_t LuaScriptEngine::s_globalScriptVersion = 0;
	bool LuaScriptEngine::s_initialized = false;

	namespace {

		bool ReadScriptFile(const std::filesystem::path& filepath, std::string& outContent)
		{
			std::ifstream file(filepath, std::ios::binary);
			if (!file.is_open())
				return false;

			std::ostringstream buffer;
			buffer << file.rdbuf();
			outContent = buffer.str();
			return true;
		}

	}

	LuaScriptEngine::~LuaScriptEngine()
	{
		delete s_luaState;
	}

	void LuaScriptEngine::Init()
	{
		if (s_initialized)
			return;

		// YUICY_CORE_INFO("LuaScriptEngine: Initializing...");
		s_luaState = new sol::state();
		// Open standard Lua libraries
		s_luaState->open_libraries(
			sol::lib::base,
			sol::lib::math,
			sol::lib::string,
			sol::lib::table,
			sol::lib::os,
			sol::lib::io,
			sol::lib::package
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

	std::string LuaScriptEngine::NormalizePath(const std::filesystem::path& filepath)
	{
		std::error_code error;
		std::filesystem::path normalized = std::filesystem::weakly_canonical(filepath, error);
		// 例如没有访问权限时 weakly_canonical 会失败，退化为纯字符串规范化，保证同一写法仍得到同一个 key
		if (error)
			normalized = filepath.lexically_normal();

		return normalized.generic_string();
	}

	bool LuaScriptEngine::CompileScript(const std::filesystem::path& filepath, const std::string& normalizedPath,
		sol::protected_function& outChunk, std::string& outError)
	{
		if (!s_initialized)
		{
			outError = "LuaScriptEngine is not initialized";
			return false;
		}

		std::string source;
		if (!ReadScriptFile(filepath, source))
		{
			outError = "Failed to open script file";
			return false;
		}

		// '@' 前缀让 Lua 把 chunkname 当作文件名，报错格式为 "<路径>:<行号>: <信息>"
		sol::load_result loadResult = s_luaState->load(source, "@" + normalizedPath);
		if (!loadResult.valid())
		{
			sol::error err = loadResult;
			outError = err.what();
			return false;
		}
		
		outChunk = loadResult.get<sol::protected_function>();
		return true;
	}

	LuaScriptEngine::ScriptEntry* LuaScriptEngine::LoadScriptEntry(const std::filesystem::path& filepath, const std::string& normalizedPath)
	{
		if (auto it = s_scriptCache.find(normalizedPath); it != s_scriptCache.end())
			return &it->second;

		sol::protected_function chunk;
		std::string error;
		if (!CompileScript(filepath, normalizedPath, chunk, error))
		{
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to load script '{}': {}", normalizedPath, error);
			return nullptr;
		}

		ScriptEntry& entry = s_scriptCache[normalizedPath];
		entry.Chunk = std::move(chunk);
		entry.Version = 1;
		YUICY_CORE_TRACE("LuaScriptEngine: Loaded script: {}", normalizedPath);
		return &entry;
	}

	bool LuaScriptEngine::LoadScript(const std::filesystem::path& filepath)
	{
		return LoadScriptEntry(filepath, NormalizePath(filepath)) != nullptr;
	}

	bool LuaScriptEngine::ReloadScript(const std::filesystem::path& filepath)
	{
		const std::string normalizedPath = NormalizePath(filepath);

		sol::protected_function chunk;
		std::string error;
		if (!CompileScript(filepath, normalizedPath, chunk, error))
		{
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to reload script '{}': {}", normalizedPath, error);
			return false;
		}

		auto [it, inserted] = s_scriptCache.try_emplace(normalizedPath);
		ScriptEntry& entry = it->second;
		entry.Chunk = std::move(chunk);
		entry.Version = inserted ? 1 : entry.Version + 1;
		++s_globalScriptVersion;

		YUICY_CORE_INFO("LuaScriptEngine: Reloaded script '{}' (version {})", normalizedPath, entry.Version);
		return true;
	}

	bool LuaScriptEngine::ValidateScript(const std::filesystem::path& filepath, std::string& outError)
	{
		outError.clear();
		sol::protected_function chunk;
		return CompileScript(filepath, NormalizePath(filepath), chunk, outError);
	}

	uint32_t LuaScriptEngine::GetScriptVersion(const std::string& normalizedPath)
	{
		if (auto it = s_scriptCache.find(normalizedPath); it != s_scriptCache.end())
			return it->second.Version;
		return 0;
	}

	sol::table LuaScriptEngine::CreateScriptInstance(const std::filesystem::path& filepath)
	{
		const std::string normalizedPath = NormalizePath(filepath);
		ScriptEntry* entry = LoadScriptEntry(filepath, normalizedPath);
		if (!entry)
			return sol::lua_nil;

		sol::protected_function_result result = entry->Chunk();
		if (!result.valid())
		{
			sol::error err = result;
			YUICY_CORE_ERROR("LuaScriptEngine: Failed to execute script '{}': {}", normalizedPath, err.what());
			return sol::lua_nil;
		}

		// 固定脚本返回值
		sol::object obj = result;
		if (!obj.is<sol::table>())
		{
			YUICY_CORE_ERROR("LuaScriptEngine: Script '{}' did not return a table", normalizedPath);
			return sol::lua_nil;
		}

		sol::table classTable = obj.as<sol::table>();
		sol::table instance = s_luaState->create_table();

		for (auto& pair : classTable)
		{
			instance[pair.first] = pair.second;
		}

		return instance;  // 返回脚本的浅拷贝实例
	}

	void LuaScriptEngine::ClearScriptCache()
	{
		s_scriptCache.clear();
		YUICY_CORE_TRACE("LuaScriptEngine: Script cache cleared");
	}

}
