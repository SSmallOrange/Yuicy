#include <Yuicy.h>
#include "DungeonTileMap.h"
#include "DungeonMapParser.h"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <fstream>
#include <sstream>

namespace TinyDungeon {

	Yuicy::Ref<Yuicy::ITileMapData> DungeonMapParser::Parse(const std::filesystem::path& filePath)
	{
		std::ifstream file(filePath);
		if (!file.is_open())
		{
			YUICY_CORE_ERROR("DungeonMapParser: Failed to open file: {}", filePath.string());
			return nullptr;
		}

		std::stringstream buffer;
		buffer << file.rdbuf();
		std::string jsonContent = buffer.str();
		file.close();

		auto mapData = Yuicy::CreateRef<TileMapData>();
		mapData->filePath = filePath.string();

		if (!DeserializeFromJson(jsonContent, *mapData))
		{
			YUICY_CORE_ERROR("DungeonMapParser: Failed to deserialize JSON");
			return nullptr;
		}

		LoadTilesetTextures(*mapData, filePath.parent_path());
		BuildSubTextures(*mapData);

		YUICY_CORE_INFO("DungeonMapParser: Loaded map '{}'", mapData->map.name);
		return mapData;
	}

	bool DungeonMapParser::DeserializeFromJson(const std::string& jsonContent, TileMapData& outData)
	{
		rapidjson::Document doc;
		rapidjson::ParseResult result = doc.Parse(jsonContent.c_str());

		if (!result)
		{
			YUICY_CORE_ERROR("JSON parse error: {} (offset: {})",
				rapidjson::GetParseError_En(result.Code()), result.Offset());
			return false;
		}

		// 解析 header
		if (doc.HasMember("header") && doc["header"].IsObject())
		{
			const auto& header = doc["header"];
			if (header.HasMember("exportTime")) outData.header.exportTime = header["exportTime"].GetString();
			if (header.HasMember("generator")) outData.header.generator = header["generator"].GetString();
			if (header.HasMember("version")) outData.header.version = header["version"].GetString();
		}

		// 解析 map 信息
		if (doc.HasMember("map") && doc["map"].IsObject())
		{
			const auto& map = doc["map"];
			if (map.HasMember("name")) outData.map.name = map["name"].GetString();
			if (map.HasMember("width")) outData.map.width = map["width"].GetInt();
			if (map.HasMember("height")) outData.map.height = map["height"].GetInt();
			if (map.HasMember("tileWidth")) outData.map.tileWidth = map["tileWidth"].GetInt();
			if (map.HasMember("tileHeight")) outData.map.tileHeight = map["tileHeight"].GetInt();

			if (map.HasMember("pixelSize") && map["pixelSize"].IsObject())
			{
				const auto& pixelSize = map["pixelSize"];
				if (pixelSize.HasMember("height")) outData.map.pixelSize.height = pixelSize["height"].GetInt();
				if (pixelSize.HasMember("width")) outData.map.pixelSize.width = pixelSize["width"].GetInt();
			}
		}

		// 解析 layers
		if (doc.HasMember("layers") && doc["layers"].IsArray())
		{
			for (const auto& layerJson : doc["layers"].GetArray())
			{
				TileLayer layer;
				if (layerJson.HasMember("id")) layer.id = layerJson["id"].GetInt();
				if (layerJson.HasMember("name")) layer.name = layerJson["name"].GetString();
				if (layerJson.HasMember("visible")) layer.visible = layerJson["visible"].GetBool();
				if (layerJson.HasMember("locked")) layer.locked = layerJson["locked"].GetBool();
				if (layerJson.HasMember("opacity")) layer.opacity = layerJson["opacity"].GetFloat();
				if (layerJson.HasMember("tileCount")) layer.tileCount = layerJson["tileCount"].GetInt();

				// 解析 tiles
				if (layerJson.HasMember("tiles") && layerJson["tiles"].IsArray())
				{
					for (const auto& tileJson : layerJson["tiles"].GetArray())
					{
						TileInstance tile;
						if (tileJson.HasMember("displayName")) tile.displayName = tileJson["displayName"].GetString();
						if (tileJson.HasMember("sliceId")) tile.sliceId = tileJson["sliceId"].GetString();
						if (tileJson.HasMember("sliceIndex")) tile.sliceIndex = tileJson["sliceIndex"].GetInt();
						if (tileJson.HasMember("tilesetId")) tile.tilesetId = tileJson["tilesetId"].GetString();
						if (tileJson.HasMember("layer")) tile.layer = tileJson["layer"].GetInt();
						if (tileJson.HasMember("zIndex")) tile.zIndex = tileJson["zIndex"].GetInt();

						// 解析 position
						if (tileJson.HasMember("position") && tileJson["position"].IsObject())
						{
							const auto& pos = tileJson["position"];
							if (pos.HasMember("gridX")) tile.position.gridX = pos["gridX"].GetInt();
							if (pos.HasMember("gridY")) tile.position.gridY = pos["gridY"].GetInt();
							if (pos.HasMember("pixelX")) tile.position.pixelX = pos["pixelX"].GetInt();
							if (pos.HasMember("pixelY")) tile.position.pixelY = pos["pixelY"].GetInt();
						}

						// 解析 size
						if (tileJson.HasMember("size") && tileJson["size"].IsObject())
						{
							const auto& size = tileJson["size"];
							if (size.HasMember("gridWidth")) tile.size.gridWidth = size["gridWidth"].GetInt();
							if (size.HasMember("gridHeight")) tile.size.gridHeight = size["gridHeight"].GetInt();
							if (size.HasMember("pixelWidth")) tile.size.pixelWidth = size["pixelWidth"].GetInt();
							if (size.HasMember("pixelHeight")) tile.size.pixelHeight = size["pixelHeight"].GetInt();
						}

						// 解析 transform
						if (tileJson.HasMember("transform") && tileJson["transform"].IsObject())
						{
							const auto& transform = tileJson["transform"];
							if (transform.HasMember("flipX")) tile.transform.flipX = transform["flipX"].GetBool();
							if (transform.HasMember("flipY")) tile.transform.flipY = transform["flipY"].GetBool();
							if (transform.HasMember("rotation")) tile.transform.rotation = transform["rotation"].GetFloat();
						}

						// 解析 collision
						if (tileJson.HasMember("collision") && tileJson["collision"].IsObject())
						{
							const auto& collision = tileJson["collision"];
							if (collision.HasMember("enabled")) tile.collision.enabled = collision["enabled"].GetBool();
							if (collision.HasMember("type")) tile.collision.type = collision["type"].GetString();
							if (collision.HasMember("typeId")) tile.collision.typeId = collision["typeId"].GetInt();
						}

						// 解析 tags
						if (tileJson.HasMember("tags") && tileJson["tags"].IsArray())
						{
							for (const auto& tag : tileJson["tags"].GetArray())
							{
								if (tag.IsString())
									tile.tags.push_back(tag.GetString());
							}
						}

						// 解析 customData
						if (tileJson.HasMember("customData") && tileJson["customData"].IsObject())
						{
							for (auto it = tileJson["customData"].MemberBegin();
								it != tileJson["customData"].MemberEnd(); ++it)
							{
								if (it->value.IsString())
									tile.customData[it->name.GetString()] = it->value.GetString();
							}
						}

						layer.tiles.push_back(std::move(tile));
					}
				}
				outData.layers.push_back(std::move(layer));
			}
		}

		// 解析 slices
		if (doc.HasMember("slices") && doc["slices"].IsArray())
		{
			for (const auto& sliceJson : doc["slices"].GetArray())
			{
				SliceDef slice;
				if (sliceJson.HasMember("id")) slice.id = sliceJson["id"].GetString();
				if (sliceJson.HasMember("index")) slice.index = sliceJson["index"].GetInt();
				if (sliceJson.HasMember("name")) slice.name = sliceJson["name"].GetString();
				if (sliceJson.HasMember("group")) slice.group = sliceJson["group"].GetString();
				if (sliceJson.HasMember("decorationOnly")) slice.decorationOnly = sliceJson["decorationOnly"].GetBool();

				// 解析 anchor
				if (sliceJson.HasMember("anchor") && sliceJson["anchor"].IsObject())
				{
					const auto& anchor = sliceJson["anchor"];
					if (anchor.HasMember("x")) slice.anchor.x = anchor["x"].GetFloat();
					if (anchor.HasMember("y")) slice.anchor.y = anchor["y"].GetFloat();
				}

				// 解析 sourceRect
				if (sliceJson.HasMember("sourceRect") && sliceJson["sourceRect"].IsObject())
				{
					const auto& rect = sliceJson["sourceRect"];
					if (rect.HasMember("x")) slice.sourceRect.x = rect["x"].GetInt();
					if (rect.HasMember("y")) slice.sourceRect.y = rect["y"].GetInt();
					if (rect.HasMember("width")) slice.sourceRect.width = rect["width"].GetInt();
					if (rect.HasMember("height")) slice.sourceRect.height = rect["height"].GetInt();
				}

				outData.slices.push_back(std::move(slice));
			}
		}

		// 解析 tilesets
		if (doc.HasMember("tilesets") && doc["tilesets"].IsArray())
		{
			for (const auto& tilesetJson : doc["tilesets"].GetArray())
			{
				TileSet tileset;
				if (tilesetJson.HasMember("id")) tileset.id = tilesetJson["id"].GetInt();
				if (tilesetJson.HasMember("name")) tileset.name = tilesetJson["name"].GetString();
				if (tilesetJson.HasMember("imagePath")) tileset.imagePath = tilesetJson["imagePath"].GetString();
				if (tilesetJson.HasMember("imageWidth")) tileset.imageWidth = tilesetJson["imageWidth"].GetInt();
				if (tilesetJson.HasMember("imageHeight")) tileset.imageHeight = tilesetJson["imageHeight"].GetInt();

				// 解析 tileset slices
				if (tilesetJson.HasMember("slices") && tilesetJson["slices"].IsArray())
				{
					for (const auto& sliceJson : tilesetJson["slices"].GetArray())
					{
						TilesetSlice slice;
						if (sliceJson.HasMember("id")) slice.id = sliceJson["id"].GetString();
						if (sliceJson.HasMember("name")) slice.name = sliceJson["name"].GetString();
						if (sliceJson.HasMember("group")) slice.group = sliceJson["group"].GetString();
						if (sliceJson.HasMember("tags")) slice.tags = sliceJson["tags"].GetString();
						if (sliceJson.HasMember("x")) slice.x = sliceJson["x"].GetInt();
						if (sliceJson.HasMember("y")) slice.y = sliceJson["y"].GetInt();
						if (sliceJson.HasMember("width")) slice.width = sliceJson["width"].GetInt();
						if (sliceJson.HasMember("height")) slice.height = sliceJson["height"].GetInt();
						if (sliceJson.HasMember("isCollision")) slice.isCollision = sliceJson["isCollision"].GetBool();
						if (sliceJson.HasMember("collisionType")) slice.collisionType = sliceJson["collisionType"].GetInt();
						if (sliceJson.HasMember("isDecorationOnly")) slice.isDecorationOnly = sliceJson["isDecorationOnly"].GetBool();

						// 解析 anchor
						if (sliceJson.HasMember("anchor") && sliceJson["anchor"].IsObject())
						{
							const auto& anchor = sliceJson["anchor"];
							if (anchor.HasMember("x")) slice.anchor.x = anchor["x"].GetFloat();
							if (anchor.HasMember("y")) slice.anchor.y = anchor["y"].GetFloat();
						}

						tileset.slices.push_back(std::move(slice));
					}
				}

				outData.tilesets.push_back(std::move(tileset));
			}
		}

		return true;
	}

	void DungeonMapParser::LoadTilesetTextures(TileMapData& mapData, const std::filesystem::path& basePath)
	{
		for (auto& tileset : mapData.tilesets)
		{
			std::filesystem::path texturePath = basePath / tileset.imagePath;

			if (!std::filesystem::exists(texturePath))
			{
				texturePath = tileset.imagePath;
			}

			if (std::filesystem::exists(texturePath))
			{
				tileset.texture = Yuicy::Texture2D::Create(texturePath.string());
				YUICY_CORE_TRACE("DungeonMapParser: Loaded texture '{}'", tileset.name);
			}
			else
			{
				YUICY_CORE_WARN("DungeonMapParser: Texture not found: {}", texturePath.string());
			}
		}
	}

	void DungeonMapParser::BuildSubTextures(TileMapData& mapData)
	{
		for (auto& tileset : mapData.tilesets)
		{
			 if (!tileset.texture)
				 continue;

			float texWidth = static_cast<float>(tileset.imageWidth);
			float texHeight = static_cast<float>(tileset.imageHeight);

			for (auto& slice : tileset.slices)
			{
				float flippedY = texHeight - static_cast<float>(slice.y) - static_cast<float>(slice.height);

				glm::vec2 min = {
					static_cast<float>(slice.x) / texWidth,
					flippedY / texHeight
				};
				glm::vec2 max = {
					static_cast<float>(slice.x + slice.width) / texWidth,
					(flippedY + static_cast<float>(slice.height)) / texHeight
				};

				slice.subTexture = Yuicy::CreateRef<Yuicy::SubTexture2D>(tileset.texture, min, max);
			}
		}
	}
}
