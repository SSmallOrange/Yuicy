#include "pch.h"
#include "Yuicy/Scripting/ScriptingAssetLoaders.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Scripting/LuaScriptAsset.h"
#include "Yuicy/Scripting/LuaScriptEngine.h"

namespace Yuicy {

	namespace {

		class LuaScriptAssetLoader : public AssetLoader
		{
		public:
			Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const override
			{
				// 总是重新编译：ReloadData 与 Reimport 都经由这里，需要绕过 LuaScriptEngine 的缓存
				if (!LuaScriptEngine::ReloadScript(absolutePath))
					return nullptr;

				return CreateRef<LuaScriptAsset>(absolutePath);
			}
		};

	}

	void RegisterScriptingAssetLoaders(AssetLoaderRegistry& registry)
	{
		registry.Register(AssetType::LuaScript, CreateScope<LuaScriptAssetLoader>());
	}

}
