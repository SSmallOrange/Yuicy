#pragma once

#include <Yuicy.h>
#include "DungeonTileMap.h"

namespace TinyDungeon {

	class DungeonMapParser : public Yuicy::ITileMapParser
	{
	public:
		DungeonMapParser() = default;
		~DungeonMapParser() override = default;

		Yuicy::Ref<Yuicy::ITileMapData> Parse(const std::filesystem::path& filePath) override;
		std::vector<std::string> GetSupportedExtensions() const override { return { ".json" }; }

	private:
		bool DeserializeFromJson(const std::string& jsonContent, TileMapData& outData);

		void LoadTilesetTextures(TileMapData& mapData, const std::filesystem::path& basePath);
		void BuildSubTextures(TileMapData& mapData);
	};

}