#include "pch.h"
#include "Yuicy/Renderer/RendererAssetLoaders.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/Texture.h"

namespace Yuicy {

	namespace {

		class TextureAssetLoader : public AssetLoader
		{
		public:
			Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const override
			{
				Ref<Texture2D> texture = Texture2D::Create(absolutePath.string());
				if (!texture)
					YUICY_CORE_ERROR("[AssetLoader] Failed to load texture from: {}", absolutePath.string());

				return texture;
			}
		};

		class ShaderAssetLoader : public AssetLoader
		{
		public:
			Ref<Asset> Load(const AssetMetadata& metadata, const std::filesystem::path& absolutePath) const override
			{
				Ref<Shader> shader = Shader::Create(absolutePath.string());
				if (!shader)
					YUICY_CORE_ERROR("[AssetLoader] Failed to load shader from: {}", absolutePath.string());

				return shader;
			}
		};

	}

	void RegisterRendererAssetLoaders(AssetLoaderRegistry& registry)
	{
		registry.Register(AssetType::Texture, CreateScope<TextureAssetLoader>());
		registry.Register(AssetType::Shader, CreateScope<ShaderAssetLoader>());
	}

}
