#pragma once

#include <cstdint>
#include <filesystem>
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

		// 编译并缓存脚本，不创建实例；已缓存时直接返回 true，不会重新读取文件
		static bool LoadScript(const std::filesystem::path& filepath);
		// 复制Lua脚本实例
		static sol::table CreateScriptInstance(const std::filesystem::path& filepath);
		// 无论是否已缓存都重新读取并编译；失败时保留缓存中的旧版本并返回 false
		static bool ReloadScript(const std::filesystem::path& filepath);
		// 只做语法检查，不修改缓存；成功时清空 outError，失败时写入 Lua 给出的错误信息（含行号）
		static bool ValidateScript(const std::filesystem::path& filepath, std::string& outError);
		// 清理 Lua 脚本缓存
		static void ClearScriptCache();

		// 脚本缓存的 key：解析符号链接后的绝对路径，分隔符统一为 '/'。会访问文件系统，不要每帧调用
		static std::string NormalizePath(const std::filesystem::path& filepath);
		// 参数必须是 NormalizePath 的结果。未缓存返回 0，首次加载为 1，之后每次 ReloadScript 成功加 1
		static uint32_t GetScriptVersion(const std::string& normalizedPath);
		// 任意脚本 ReloadScript 成功即加 1，用于每帧快速判断是否需要逐个比对 GetScriptVersion
		static uint32_t GetGlobalScriptVersion() { return s_globalScriptVersion; }

	private:
		struct ScriptEntry
		{
			sol::protected_function Chunk;  // 执行后返回脚本的 class table
			uint32_t Version = 0;
		};

		static void RegisterBindings();
		// 返回 nullptr 表示加载失败；返回的指针在缓存下一次增删前有效
		static ScriptEntry* LoadScriptEntry(const std::filesystem::path& filepath, const std::string& normalizedPath);
		static bool CompileScript(const std::filesystem::path& filepath, const std::string& normalizedPath,
			sol::protected_function& outChunk, std::string& outError);

	private:
		static sol::state* s_luaState;
		static std::unordered_map<std::string, ScriptEntry> s_scriptCache;
		static uint32_t s_globalScriptVersion;
		static bool s_initialized;
	};

}
