#include "pch.h"
#include "Yuicy/Project/ProjectSerializer.h"

#include "yaml-cpp/yaml.h"

#include <fstream>
#include <sstream>

namespace Yuicy {

	// 路径以 UTF-8 存储；path::string() 在 Windows 上使用 ANSI 代码页
	static std::string PathToYaml(const std::filesystem::path& path)
	{
		const std::u8string utf8 = path.generic_u8string();
		return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
	}

	// 键缺失或值为空时返回 std::nullopt；yaml-cpp 对空值调用 as<std::string>() 返回 "null"
	static std::optional<std::filesystem::path> ReadOptionalPath(const YAML::Node& node)
	{
		if (!node || node.IsNull())
			return std::nullopt;

		const std::string text = node.as<std::string>();
		return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
	}

	static void SerializeRenderer2D(YAML::Emitter& out, const Renderer2DSettings& settings)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "SortingLayers" << YAML::Value << YAML::BeginSeq;
		for (const auto& layer : settings.SortingLayers.Layers)
		{
			out << YAML::Flow << YAML::BeginMap;
			out << YAML::Key << "Name" << YAML::Value << layer.Name;
			out << YAML::Key << "Order" << YAML::Value << layer.Order;
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	static void SerializePhysics2D(YAML::Emitter& out, const Physics2DSettings& settings)
	{
		out << YAML::BeginMap;
		out << YAML::Key << "CollisionLayers" << YAML::Value << YAML::Flow << YAML::BeginSeq;
		for (const std::string& layerName : settings.CollisionLayers.LayerNames)
			out << layerName;
		out << YAML::EndSeq;
		out << YAML::EndMap;
	}

	bool ProjectSerializer::Serialize(const ProjectSettings& settings, const std::filesystem::path& filepath)
	{
		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Project" << YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Name" << YAML::Value << settings.Name;
		out << YAML::Key << "AssetDirectory" << YAML::Value << PathToYaml(settings.AssetDirectory);
		out << YAML::Key << "StartScene" << YAML::Value << PathToYaml(settings.StartScene);
		out << YAML::EndMap;

		out << YAML::Key << "Renderer2D" << YAML::Value;
		SerializeRenderer2D(out, settings.Renderer2D);

		out << YAML::Key << "Physics2D" << YAML::Value;
		SerializePhysics2D(out, settings.Physics2D);

		out << YAML::EndMap;

		if (!out.good())
		{
			YUICY_CORE_ERROR("[Project] Failed to emit project file '{}': {}", filepath.string(), out.GetLastError());
			return false;
		}

		std::ofstream fout(filepath);
		if (!fout.is_open())
		{
			YUICY_CORE_ERROR("[Project] Failed to save project file: {}", filepath.string());
			return false;
		}

		fout << out.c_str();
		if (!fout)
		{
			YUICY_CORE_ERROR("[Project] Failed to write project file: {}", filepath.string());
			return false;
		}

		YUICY_CORE_INFO("[Project] Saved project: {}", filepath.string());
		return true;
	}

	// 结构错误时记录错误并返回 false；值的类型错误由 yaml-cpp 抛出 YAML::Exception，交给调用方处理
	static bool DeserializeRenderer2D(const YAML::Node& node, Renderer2DSettings& settings, const std::filesystem::path& filepath)
	{
		const YAML::Node sortingLayersNode = node["SortingLayers"];
		if (!sortingLayersNode)
			return true;

		if (!sortingLayersNode.IsSequence())
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, 'Renderer2D.SortingLayers' must be a sequence: {}", filepath.string());
			return false;
		}

		settings.SortingLayers.Layers.clear();
		for (const YAML::Node layerNode : sortingLayersNode)
			settings.SortingLayers.Layers.push_back({ layerNode["Name"].as<std::string>(), layerNode["Order"].as<int>() });

		return true;
	}

	static bool DeserializePhysics2D(const YAML::Node& node, Physics2DSettings& settings, const std::filesystem::path& filepath)
	{
		const YAML::Node collisionLayersNode = node["CollisionLayers"];
		if (!collisionLayersNode)
			return true;

		if (!collisionLayersNode.IsSequence())
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, 'Physics2D.CollisionLayers' must be a sequence: {}", filepath.string());
			return false;
		}

		if (collisionLayersNode.size() > CollisionLayerConfig::MaxLayers)
		{
			YUICY_CORE_WARN("[Project] 'Physics2D.CollisionLayers' has {} entries, only the first {} are used: {}",
				collisionLayersNode.size(), CollisionLayerConfig::MaxLayers, filepath.string());
		}

		// 条目少于 MaxLayers 时，其余层保留默认名
		const size_t count = std::min(collisionLayersNode.size(), static_cast<size_t>(CollisionLayerConfig::MaxLayers));
		for (size_t i = 0; i < count; i++)
			settings.CollisionLayers.LayerNames[i] = collisionLayersNode[i].as<std::string>();

		return true;
	}

	static std::optional<ProjectSettings> DeserializeProjectSettings(const YAML::Node& data, const std::filesystem::path& filepath)
	{
		const YAML::Node projectNode = data.IsMap() ? data["Project"] : YAML::Node();
		if (!projectNode || !projectNode.IsMap())
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, missing 'Project' section: {}", filepath.string());
			return std::nullopt;
		}

		const YAML::Node nameNode = projectNode["Name"];
		if (!nameNode || !nameNode.IsScalar())
		{
			YUICY_CORE_ERROR("[Project] Invalid project file, missing project name: {}", filepath.string());
			return std::nullopt;
		}

		ProjectSettings settings;
		settings.Name = nameNode.as<std::string>();
		if (std::optional<std::filesystem::path> assetDirectory = ReadOptionalPath(projectNode["AssetDirectory"]))
			settings.AssetDirectory = std::move(*assetDirectory);
		if (std::optional<std::filesystem::path> startScene = ReadOptionalPath(projectNode["StartScene"]))
			settings.StartScene = std::move(*startScene);

		if (const YAML::Node renderer2DNode = data["Renderer2D"])
		{
			if (!DeserializeRenderer2D(renderer2DNode, settings.Renderer2D, filepath))
				return std::nullopt;
		}

		if (const YAML::Node physics2DNode = data["Physics2D"])
		{
			if (!DeserializePhysics2D(physics2DNode, settings.Physics2D, filepath))
				return std::nullopt;
		}

		return settings;
	}

	std::optional<ProjectSettings> ProjectSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			YUICY_CORE_ERROR("[Project] Failed to open project file: {}", filepath.string());
			return std::nullopt;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		std::optional<ProjectSettings> settings;
		try
		{
			settings = DeserializeProjectSettings(YAML::Load(strStream.str()), filepath);
		}
		catch (const YAML::Exception& e)
		{
			YUICY_CORE_ERROR("[Project] Failed to parse project file '{}': {}", filepath.string(), e.what());
			return std::nullopt;
		}

		if (settings)
			YUICY_CORE_INFO("[Project] Loaded project '{}' from: {}", settings->Name, filepath.string());

		return settings;
	}
}
