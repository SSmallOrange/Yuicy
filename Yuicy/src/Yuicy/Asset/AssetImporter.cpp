#include "pch.h"
#include "AssetImporter.h"

#include "Yuicy/Project/Project.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/SceneSerializer.h"
#include "Yuicy/Sprite/SpriteAsset.h"
#include "Yuicy/Tilemap/Tile.h"
#include "Yuicy/Utilities/YAMLSerializationHelpers.h"

#include "yaml-cpp/yaml.h"

#include <fstream>

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

	// SpriteAssetSerializer
	class SpriteAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			auto sprite = std::dynamic_pointer_cast<SpriteAsset>(asset);
			if (!sprite)
				return;

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Sprite";
			out << YAML::BeginMap;
			out << YAML::Key << "TextureHandle" << YAML::Value << (uint64_t)sprite->m_textureHandle;
			out << YAML::Key << "UVMin" << YAML::Value << sprite->m_uvMin;
			out << YAML::Key << "UVMax" << YAML::Value << sprite->m_uvMax;
			out << YAML::Key << "Pivot" << YAML::Value << sprite->m_pivot;
			out << YAML::Key << "Border" << YAML::Value << sprite->m_border;
			out << YAML::Key << "PixelsPerUnit" << YAML::Value << sprite->m_pixelsPerUnit;
			out << YAML::EndMap;
			out << YAML::EndMap;

			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			std::ofstream fout(absolutePath);
			fout << out.c_str();
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			std::ifstream stream(absolutePath);
			if (!stream.is_open())
			{
				YUICY_CORE_ERROR("SpriteAssetSerializer: Failed to open sprite asset: {0}", absolutePath.string());
				return false;
			}

			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());
			YAML::Node spriteNode = data["Sprite"] ? data["Sprite"] : data;

			auto sprite = CreateRef<SpriteAsset>();
			sprite->handle = metadata.handle;
			sprite->m_textureHandle = spriteNode["TextureHandle"].as<uint64_t>(0);
			sprite->m_uvMin = spriteNode["UVMin"].as<glm::vec2>(glm::vec2(0.0f));
			sprite->m_uvMax = spriteNode["UVMax"].as<glm::vec2>(glm::vec2(1.0f));
			sprite->m_pivot = spriteNode["Pivot"].as<glm::vec2>(glm::vec2(0.5f));
			sprite->m_border = spriteNode["Border"].as<glm::vec4>(glm::vec4(0.0f));
			sprite->m_pixelsPerUnit = spriteNode["PixelsPerUnit"].as<float>(100.0f);

			asset = sprite;
			return true;
		}
	};

	// TileAssetSerializer
	class TileAssetSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override
		{
			auto tile = std::dynamic_pointer_cast<TileAsset>(asset);
			if (!tile)
				return;

			YAML::Emitter out;
			out << YAML::BeginMap;
			out << YAML::Key << "Tile";
			out << YAML::BeginMap;
			out << YAML::Key << "SpriteHandle" << YAML::Value << (uint64_t)tile->m_spriteHandle;
			out << YAML::Key << "Color" << YAML::Value << tile->m_color;
			out << YAML::Key << "ColliderType" << YAML::Value << TilemapUtils::TileColliderTypeToString(tile->m_colliderType);
			out << YAML::Key << "Flags" << YAML::Value << tile->m_flags;
			out << YAML::EndMap;
			out << YAML::EndMap;

			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			std::ofstream fout(absolutePath);
			fout << out.c_str();
		}

		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override
		{
			auto absolutePath = Project::GetActiveAssetDirectory() / metadata.filePath;
			std::ifstream stream(absolutePath);
			if (!stream.is_open())
			{
				YUICY_CORE_ERROR("TileAssetSerializer: Failed to open tile asset: {0}", absolutePath.string());
				return false;
			}

			std::stringstream strStream;
			strStream << stream.rdbuf();

			YAML::Node data = YAML::Load(strStream.str());
			YAML::Node tileNode = data["Tile"] ? data["Tile"] : data;

			auto tile = CreateRef<TileAsset>();
			tile->handle = metadata.handle;
			tile->m_spriteHandle = tileNode["SpriteHandle"].as<uint64_t>(0);
			tile->m_color = tileNode["Color"].as<glm::vec4>(glm::vec4(1.0f));
			tile->m_colliderType = TilemapUtils::TileColliderTypeFromString(tileNode["ColliderType"].as<std::string>("None"));
			tile->m_flags = tileNode["Flags"].as<uint32_t>(0);

			asset = tile;
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
		s_serializers[AssetType::Sprite]    = CreateScope<SpriteAssetSerializer>();
		s_serializers[AssetType::Tile]      = CreateScope<TileAssetSerializer>();
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
