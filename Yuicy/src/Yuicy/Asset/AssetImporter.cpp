#include "pch.h"
#include "AssetImporter.h"

#include "Yuicy/Project/Project.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/SceneSerializer.h"

namespace Yuicy {
	// TextureAssetSerializer
	class TextureAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			// Texture 无需序列化
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			
			if (asset = Texture2D::Create(absolutePath.string()); asset)
			{
				asset->handle = metadata.handle;
				return true;
			}
			
			YUICY_CORE_ERROR("TextureAssetSerializer: Failed to load texture from: {0}", absolutePath.string());
			return false;
		}
	};
	
	// SceneAssetSerializer
	class SceneAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			auto scene = std::dynamic_pointer_cast<Scene>(asset);
			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			SceneSerializer serializer(scene);
			serializer.Serialize(absolutePath);
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			// 仅创建空场景并赋值 Handle，实际反序列化在编辑器打开场景时进行
			asset = CreateRef<Scene>();
			asset->handle = metadata.handle;
			return true;
		}
	};

	// FontAssetSerializer
	class FontAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			// TODO
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			// TODO
			YUICY_CORE_WARN("FontAssetSerializer: Font loading not yet implemented for: {0}", metadata.filePath.string());
			return false;
		}
	};

	// ShaderAssetSerializer
	class ShaderAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			// Shader 无需序列化
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			
			if (asset = Shader::Create(absolutePath.string()); asset)
			{
				asset->handle = metadata.handle;
				return true;
			}

			YUICY_CORE_ERROR("ShaderAssetSerializer: Failed to load shader from: {0}", absolutePath.string());
			return false;
		}
	};

	// LuaScriptAssetSerializer
	class LuaScriptAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			// TODO
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			// TODO
			YUICY_CORE_WARN("LuaScriptAssetSerializer: Lua script loading not yet implemented for: {0}", metadata.filePath.string());
			return false;
		}
	};

	// AssetImporter
	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_serializers;

	void AssetImporter::Init()
	{
		s_serializers.clear();
		s_serializers[AssetType::Texture]   = CreateScope<TextureAssetSerializer>();
		s_serializers[AssetType::Scene]     = CreateScope<SceneAssetSerializer>();
		s_serializers[AssetType::Font]      = CreateScope<FontAssetSerializer>();
		s_serializers[AssetType::Shader]    = CreateScope<ShaderAssetSerializer>();
		s_serializers[AssetType::LuaScript] = CreateScope<LuaScriptAssetSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (s_serializers.find(metadata.type) == s_serializers.end())
		{
			YUICY_CORE_WARN("AssetImporter: No serializer available for asset type: {0}", metadata.filePath.stem().string());
			return;
		}

		s_serializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}

	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		if (s_serializers.find(metadata.type) == s_serializers.end())
		{
			YUICY_CORE_WARN("AssetImporter: No importer available for asset type: {0}", metadata.filePath.stem().string());
			return false;
		}

		return s_serializers[metadata.type]->TryLoadData(metadata, asset);
	}

}
