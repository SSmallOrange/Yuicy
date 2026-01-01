#pragma once

#include "Yuicy/TileMap/TileMapCommon.h"
#include <filesystem>

namespace Yuicy {

	class ITileMapParser
	{
	public:
		virtual ~ITileMapParser() = default;
		virtual Ref<ITileMapData> Parse(const std::filesystem::path& filePath) = 0;
		virtual std::vector<std::string> GetSupportedExtensions() const = 0;
	};

	class TileMapLoader
	{
	public:
		TileMapLoader();
		~TileMapLoader() = default;

		Ref<ITileMapData> Load(const std::filesystem::path& filePath);
		void RegisterParser(Ref<ITileMapParser> parser);

		template<typename T>
		Ref<T> LoadAs(const std::filesystem::path& filePath)
		{
			auto data = Load(filePath);
			return std::dynamic_pointer_cast<T>(data);
		}

		static TileMapLoader& Get();

	private:
		std::unordered_map<std::string, Ref<ITileMapParser>> m_parsers;  // 以拓展名区分解析器
		ITileMapParser* FindParser(const std::string& extension);
	};
}
