#include "pch.h"
#include "Yuicy/Asset/AssetRegistrySerializer.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <map>
#include <sstream>

namespace Yuicy {

	// path::string() 在 Windows 上使用 ANSI 代码页
	static std::string PathToYaml(const std::filesystem::path& path)
	{
		const std::u8string utf8 = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
	}

	static std::filesystem::path PathFromYaml(const std::string& text)
	{
		return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
	}

	bool AssetRegistrySerializer::Serialize(const AssetRegistry& registry, const std::filesystem::path& filepath)
	{
		std::map<uint64_t, const AssetMetadata*> sortedEntries;
		for (const auto& [handle, metadata] : registry)
			sortedEntries[static_cast<uint64_t>(handle)] = &metadata;

		YAML::Emitter out;
		out << YAML::BeginMap;
		out << YAML::Key << "Assets" << YAML::Value << YAML::BeginSeq;
		for (const auto& [handle, metadata] : sortedEntries)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << PathToYaml(metadata->filePath);
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(metadata->type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
		if (!fout)
		{
			YUICY_CORE_ERROR("[AssetRegistry] Failed to write {}", filepath.string());
			return false;
		}

		return true;
	}

	std::optional<AssetRegistry> AssetRegistrySerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			YUICY_CORE_ERROR("[AssetRegistry] Failed to open {}", filepath.string());
			return std::nullopt;
		}

		std::stringstream content;
		content << stream.rdbuf();

		try
		{
			const YAML::Node data = YAML::Load(content.str());
			const YAML::Node entries = data.IsMap() ? data["Assets"] : YAML::Node();
			if (!entries || !entries.IsSequence())
			{
				YUICY_CORE_ERROR("[AssetRegistry] Invalid registry, 'Assets' must be a sequence: {}", filepath.string());
				return std::nullopt;
			}

			AssetRegistry registry;
			for (const YAML::Node entry : entries)
			{
				AssetMetadata metadata;
				metadata.handle = entry["Handle"].as<uint64_t>();
				metadata.filePath = PathFromYaml(entry["FilePath"].as<std::string>());
				metadata.type = Utils::AssetTypeFromString(entry["Type"].as<std::string>());

				if (metadata.handle == 0 || metadata.type == AssetType::None)
				{
					YUICY_CORE_WARN("[AssetRegistry] Skipping entry '{}' with handle 0 or unknown type", metadata.filePath.string());
					continue;
				}

				registry.Set(metadata.handle, metadata);
			}

			return registry;
		}
		catch (const YAML::Exception& e)
		{
			YUICY_CORE_ERROR("[AssetRegistry] Failed to parse '{}': {}", filepath.string(), e.what());
			return std::nullopt;
		}
	}

}
