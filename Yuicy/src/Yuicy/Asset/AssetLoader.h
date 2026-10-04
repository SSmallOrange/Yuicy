#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Asset/AssetMetadata.h"

#include <filesystem>
#include <unordered_map>

namespace Yuicy {

	class AssetLoader
	{
	public:
		virtual ~AssetLoader() = default;

		// absolutePath 为资源文件的绝对路径；失败时返回 nullptr
		virtual Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const = 0;
	};

	class AssetLoaderRegistry
	{
	public:
		// 同一类型重复注册会触发断言
		void Register(AssetType type, Scope<AssetLoader> loader);

		// 没有对应的 Loader 或加载失败时返回 nullptr；返回的资源 handle 等于 metadata.handle
		Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const;

	private:
		std::unordered_map<AssetType, Scope<AssetLoader>> m_loaders;
	};

}
