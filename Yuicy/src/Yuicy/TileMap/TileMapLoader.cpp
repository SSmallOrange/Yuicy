#include "pch.h"
#include "Yuicy/TileMap/TileMapLoader.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Renderer/Texture.h"

#include <fstream>
#include <sstream>

namespace Yuicy {

	TileMapLoader::TileMapLoader()
	{
	}

	TileMapLoader& TileMapLoader::Get()
	{
		static TileMapLoader instance;
		return instance;
	}

	Ref<ITileMapData> TileMapLoader::Load(const std::filesystem::path& filePath)
	{
		if (!std::filesystem::exists(filePath))
		{
			YUICY_CORE_ERROR("TileMapLoader: File not found: {}", filePath.string());
			return nullptr;
		}

		std::string extension = filePath.extension().string();
		ITileMapParser* parser = FindParser(extension);

		if (!parser)
		{
			YUICY_CORE_ERROR("TileMapLoader: No parser for extension: {}", extension);
			return nullptr;
		}

		return parser->Parse(filePath);
	}

	void TileMapLoader::RegisterParser(Ref<ITileMapParser> parser)
	{
		for (const auto& ext : parser->GetSupportedExtensions())
			m_parsers[ext] = parser;
	}

	ITileMapParser* TileMapLoader::FindParser(const std::string& extension)
	{
		auto it = m_parsers.find(extension);
		return it != m_parsers.end() ? it->second.get() : nullptr;
	}
}
