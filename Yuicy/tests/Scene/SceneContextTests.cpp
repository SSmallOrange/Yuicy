#include "pch.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Project/ProjectSceneContext.h"
#include "Yuicy/Scripting/LuaScriptAsset.h"
#include "Yuicy/Scripting/LuaScriptEngine.h"

using namespace Yuicy;

TEST_SUITE("Scene")
{
	TEST_CASE("Scene loads Lua scripts through the asset manager in its context")
	{
		LuaScriptEngine::Init();
		Test::ScopedTempDirectory tempDirectory;
		const std::filesystem::path scriptFile = tempDirectory.WriteFile("Assets/Scripts/Player.lua",
			"return { Created = false, OnCreate = function(self) self.Created = true end }");

		Test::ScopedActiveProject project(tempDirectory.GetPath());
		const Ref<EditorAssetManager> assetManager = Project::GetEditorAssetManager();
		const AssetHandle scriptHandle = assetManager->GetAssetHandleFromFilePath(scriptFile);
		REQUIRE((uint64_t)scriptHandle != 0);

		Ref<Scene> scene = CreateRef<Scene>();
		scene->SetContext(MakeSceneContext(project.GetProject()->GetConfig(), assetManager));
		Entity entity = scene->CreateEntity("Player");
		entity.AddComponent<LuaScriptComponent>().ScriptHandle = scriptHandle;

		scene->OnRuntimeStart();

		auto& script = entity.GetComponent<LuaScriptComponent>();
		CHECK(script.IsLoaded);
		CHECK(script.ScriptInstance["Created"].get<bool>());

		// 热重载按文件路径找缓存：脚本只应以真实文件的 key 编译一次
		const Ref<LuaScriptAsset> asset = assetManager->GetAsset<LuaScriptAsset>(scriptHandle);
		REQUIRE(asset);
		CHECK(LuaScriptEngine::NormalizePath(asset->GetFilePath()) == LuaScriptEngine::NormalizePath(scriptFile));
		CHECK(LuaScriptEngine::GetScriptVersion(LuaScriptEngine::NormalizePath(scriptFile)) == 1);

		scene->OnRuntimeStop();
	}

	TEST_CASE("Scene without an asset manager skips Lua scripts on runtime start")
	{
		LuaScriptEngine::Init();
		Ref<Scene> scene = CreateRef<Scene>();
		REQUIRE(scene->GetContext().AssetManager.expired());

		Entity entity = scene->CreateEntity("Player");
		entity.AddComponent<LuaScriptComponent>().ScriptHandle = AssetHandle();

		scene->OnRuntimeStart();
		CHECK_FALSE(entity.GetComponent<LuaScriptComponent>().IsLoaded);
		scene->OnRuntimeStop();
	}

	// 场景只持有弱引用：关闭项目后不能让旧的资源管理器继续存活
	TEST_CASE("Scene context does not keep the asset manager alive")
	{
		Test::ScopedTempDirectory tempDirectory;
		Ref<Scene> scene = CreateRef<Scene>();
		{
			Test::ScopedActiveProject project(tempDirectory.GetPath());
			scene->SetContext(MakeSceneContext(project.GetProject()->GetConfig(), Project::GetEditorAssetManager()));
			CHECK_FALSE(scene->GetContext().AssetManager.expired());
		}

		CHECK(scene->GetContext().AssetManager.expired());
	}
}
