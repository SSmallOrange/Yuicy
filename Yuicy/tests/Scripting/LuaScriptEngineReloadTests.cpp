#include "pch.h"

#include "Yuicy/Scripting/LuaScriptEngine.h"

using namespace Yuicy;

namespace {

	int ReadValue(const std::filesystem::path& scriptPath)
	{
		sol::table instance = LuaScriptEngine::CreateScriptInstance(scriptPath);
		REQUIRE(instance.valid());
		return instance["Value"].get<int>();
	}

	std::string ToUtf8(std::u8string_view text)
	{
		return std::string(text.begin(), text.end());
	}

}

TEST_SUITE("Scripting")
{
	TEST_CASE("LoadScript keeps the cached version until ReloadScript")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const auto scriptPath = temp.WriteFile("Value.lua", "return { Value = 1 }");
		const std::string key = LuaScriptEngine::NormalizePath(scriptPath);

		CHECK(LuaScriptEngine::GetScriptVersion(key) == 0);
		CHECK(ReadValue(scriptPath) == 1);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 1);

		temp.WriteFile("Value.lua", "return { Value = 2 }");
		CHECK(LuaScriptEngine::LoadScript(scriptPath));
		CHECK(ReadValue(scriptPath) == 1);

		const uint32_t globalVersion = LuaScriptEngine::GetGlobalScriptVersion();
		CHECK(LuaScriptEngine::ReloadScript(scriptPath));
		CHECK(ReadValue(scriptPath) == 2);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 2);
		CHECK(LuaScriptEngine::GetGlobalScriptVersion() == globalVersion + 1);
	}

	TEST_CASE("ReloadScript keeps the previous version when compilation fails")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const auto scriptPath = temp.WriteFile("Broken.lua", "return { Value = 1 }");
		const std::string key = LuaScriptEngine::NormalizePath(scriptPath);
		REQUIRE(LuaScriptEngine::LoadScript(scriptPath));

		temp.WriteFile("Broken.lua", "local a = 1\nlocal = 2\n");
		const uint32_t globalVersion = LuaScriptEngine::GetGlobalScriptVersion();
		CHECK_FALSE(LuaScriptEngine::ReloadScript(scriptPath));

		CHECK(ReadValue(scriptPath) == 1);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 1);
		CHECK(LuaScriptEngine::GetGlobalScriptVersion() == globalVersion);
	}

	TEST_CASE("ReloadScript fails for a missing file without touching the cache")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const auto scriptPath = temp.WriteFile("Missing.lua", "return { Value = 1 }");
		REQUIRE(LuaScriptEngine::LoadScript(scriptPath));

		std::filesystem::remove(scriptPath);
		CHECK_FALSE(LuaScriptEngine::ReloadScript(scriptPath));
		CHECK(ReadValue(scriptPath) == 1);
	}

	TEST_CASE("ValidateScript reports the line number and leaves the cache untouched")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const auto scriptPath = temp.WriteFile("Validate.lua", "return { Value = 1 }");
		const std::string key = LuaScriptEngine::NormalizePath(scriptPath);

		std::string error = "stale";
		CHECK(LuaScriptEngine::ValidateScript(scriptPath, error));
		CHECK(error.empty());
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 0);

		temp.WriteFile("Validate.lua", "local a = 1\nlocal = 2\n");
		CHECK_FALSE(LuaScriptEngine::ValidateScript(scriptPath, error));
		CHECK_MESSAGE(error.find("Validate.lua:2:") != std::string::npos, error);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 0);
	}

	TEST_CASE("Different spellings of the same script share one cache entry")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const auto scriptPath = temp.WriteFile("Scripts/Player.lua", "return { Value = 1 }");
		temp.WriteFile("Other/.keep");
		const auto indirectPath = temp.GetPath() / "Other" / ".." / "Scripts" / "Player.lua";

		const std::string key = LuaScriptEngine::NormalizePath(scriptPath);
		CHECK(LuaScriptEngine::NormalizePath(indirectPath) == key);
		// macOS 的临时目录位于符号链接 /var 下，规范化结果应指向解析后的真实路径
		CHECK(key == std::filesystem::canonical(scriptPath).generic_string());

		REQUIRE(LuaScriptEngine::LoadScript(scriptPath));
		temp.WriteFile("Scripts/Player.lua", "return { Value = 2 }");
		CHECK(LuaScriptEngine::ReloadScript(indirectPath));

		CHECK(LuaScriptEngine::GetScriptVersion(key) == 2);
		CHECK(ReadValue(scriptPath) == 2);
	}

	TEST_CASE("Non-ASCII script paths use UTF-8 in cache keys and error messages")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory temp;
		const std::filesystem::path relativePath = std::filesystem::path(u8"脚本") / u8"玩家.lua";
		const auto scriptPath = temp.WriteFile(relativePath, "return { Value = 1 }");

		// 缓存 key 约定为 UTF-8；Windows 上 generic_string() 按 ANSI 代码页转换，预期在此暴露问题，待 Windows 回归时修复
		const std::string key = LuaScriptEngine::NormalizePath(scriptPath);
		CHECK(key == ToUtf8(std::filesystem::canonical(scriptPath).generic_u8string()));

		CHECK(ReadValue(scriptPath) == 1);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 1);

		temp.WriteFile(relativePath, "return { Value = 2 }");
		CHECK(LuaScriptEngine::ReloadScript(scriptPath));
		CHECK(ReadValue(scriptPath) == 2);
		CHECK(LuaScriptEngine::GetScriptVersion(key) == 2);

		temp.WriteFile(relativePath, "local a = 1\nlocal = 2\n");
		std::string error;
		CHECK_FALSE(LuaScriptEngine::ValidateScript(scriptPath, error));
		CHECK_MESSAGE(error.find(ToUtf8(u8"玩家.lua:2:")) != std::string::npos, error);
	}
}
