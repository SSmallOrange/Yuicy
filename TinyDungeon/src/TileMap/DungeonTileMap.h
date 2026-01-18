#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Renderer/SubTexture.h"
#include "Yuicy/TileMap/TileMapCommon.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace TinyDungeon {

	struct MapHeader
	{
		std::string exportTime;
		std::string generator;
		std::string version;
	};

	struct PixelSize
	{
		int32_t height = 0;
		int32_t width = 0;
	};

	struct MapInfo
	{
		std::string name;
		int32_t width = 0;          // 地图宽度（单元格）
		int32_t height = 0;         // 地图高度
		int32_t tileWidth = 16;     // 瓦片宽度
		int32_t tileHeight = 16;    // 瓦片高度
		PixelSize pixelSize;        // 像素尺寸
	};

	struct Anchor
	{
		float x = 0.5f;
		float y = 0.5f;
	};

	struct SourceRect
	{
		int32_t x = 0;
		int32_t y = 0;
		int32_t width = 0;
		int32_t height = 0;
	};

	struct SliceDef
	{
		std::string id;
		int32_t index = 0;
		std::string name;
		std::string group;
		Anchor anchor;
		SourceRect sourceRect;
		bool decorationOnly = false;
		
		Yuicy::Ref<Yuicy::SubTexture2D> subTexture = nullptr;
	};

	struct TilesetSlice
	{
		std::string id;
		std::string name;
		std::string group;
		std::string tags;
		Anchor anchor;
		int32_t x = 0;
		int32_t y = 0;
		int32_t width = 0;
		int32_t height = 0;
		bool isCollision = false;
		int32_t collisionType = 0;
		bool isDecorationOnly = false;

		Yuicy::Ref<Yuicy::SubTexture2D> subTexture = nullptr;
	};

	struct TileSet
	{
		int32_t id = 0;
		std::string name;
		std::string imagePath;
		int32_t imageWidth = 0;
		int32_t imageHeight = 0;
		std::vector<TilesetSlice> slices;

		Yuicy::Ref<Yuicy::Texture2D> texture = nullptr;
		Yuicy::Ref<Yuicy::SubTexture2D> GetSubTextureBySliceId(const std::string& sliceId) const;
	};

	struct TilePosition
	{
		int32_t gridX;
		int32_t gridY;
		int32_t pixelX;
		int32_t pixelY;
	};

	struct TileSize
	{
		int32_t gridWidth = 1;
		int32_t gridHeight = 1;
		int32_t pixelWidth = 16;
		int32_t pixelHeight = 16;
	};

	struct TileTransform
	{
		bool flipX = false;
		bool flipY = false;
		float rotation = 0.0f;
	};

	// 碰撞属性
	struct TileCollision
	{
		bool enabled = false;
		std::string type;           // "none", "box", "circle" 等
		int32_t typeId = 0;
	};

	struct TileInstance
	{
		std::string displayName;
		std::string sliceId;        // 关联的切片UUID
		int32_t sliceIndex = 0;
		std::string tilesetId;      // 关联的tileset名称
		int32_t layer = 0;
		int32_t zIndex = 0;

		TilePosition position;
		TileSize size;
		TileTransform transform;
		TileCollision collision;

		std::vector<std::string> tags;
		std::unordered_map<std::string, std::string> customData;
	};

	struct TileLayer
	{
		int32_t id = 0;
		std::string name;
		bool visible = true;
		bool locked = false;
		float opacity = 1.0f;
		int32_t tileCount = 0;
		std::vector<TileInstance> tiles;
	};

	struct TileMapData : public Yuicy::ITileMapData
	{
		MapHeader header;
		MapInfo map;
		std::vector<TileLayer> layers;
		std::vector<SliceDef> slices;
		std::vector<TileSet> tilesets;

		glm::vec2 GetWorldSize() const override { return { static_cast<float>(map.width), static_cast<float>(map.height) }; }
		int32_t GetTileWidth() const override { return map.tileWidth; }
		int32_t GetTileHeight() const override { return map.tileHeight; }
		const std::string& GetName() const override { return map.name; }

		TileSet* FindTileSetByName(const std::string& name);
		SliceDef* FindSliceById(const std::string& sliceId);
		TilesetSlice* FindTilesetSliceById(const std::string& sliceId);
	};

	struct TileMapDataParse
	{
		MapHeader header;
		MapInfo map;
		std::vector<TileLayer> layers;
		std::vector<SliceDef> slices;
		std::vector<TileSet> tilesets;

		void ToTileMapData(TileMapData& outData) const
		{
			outData.header = header;
			outData.map = map;
			outData.layers = layers;
			outData.slices = slices;
			outData.tilesets = tilesets;
		}
	};
}
