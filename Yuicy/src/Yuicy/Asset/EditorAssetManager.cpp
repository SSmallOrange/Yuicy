#include "pch.h"
#include "EditorAssetManager.h"

#include "Yuicy/Asset/AssetExtensions.h"
#include "Yuicy/Project/Project.h"

#include <map>
#include <ranges>
#include <fstream>
#include <yaml-cpp/yaml.h>

namespace Yuicy {

	static AssetMetadata s_nullMetadata;

	EditorAssetManager::EditorAssetManager()
	{
		AssetImporter::Init();
		LoadAssetRegistry();
		ReloadAssets();
	}

	EditorAssetManager::~EditorAssetManager()
	{
		WriteRegistryToFile();
	}

	AssetType EditorAssetManager::GetAssetType(AssetHandle assetHandle)
	{
		if (!IsAssetHandleValid(assetHandle))
			return AssetType::None;

		if (IsMemoryAsset(assetHandle))
			return GetAsset(assetHandle)->GetAssetType();

		const auto& metadata = GetMetadata(assetHandle);
		return metadata.type;
	}

	Ref<Asset> EditorAssetManager::GetAsset(AssetHandle assetHandle)
	{
		// 先检查 memory-only 资源
		if (auto it = m_memoryAssets.find(assetHandle); it != m_memoryAssets.end())
			return it->second;

		// 检查元数据是否有效
		const auto& metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
			return nullptr;

		// 直接返回已加载资源
		if (metadata.isDataLoaded)
		{
			if (auto it = m_loadedAssets.find(assetHandle); it != m_loadedAssets.end())
				return it->second;
		}

		// 未加载
		Ref<Asset> asset = nullptr;
		if (AssetImporter::TryLoadData(metadata, asset))
		{
			AssetMetadata updatedMetadata = metadata;
			updatedMetadata.isDataLoaded = true;
			m_assetRegistry.Set(assetHandle, updatedMetadata);

			m_loadedAssets[assetHandle] = asset;

			YUICY_CORE_INFO("[AssetManager] Loaded asset: {0}", metadata.filePath.string());
		}
		else
		{
			YUICY_CORE_ERROR("[AssetManager] Failed to load asset: {0}", metadata.filePath.string());
		}

		return asset;
	}

	bool EditorAssetManager::IsAssetHandleValid(AssetHandle assetHandle)
	{
		return (m_memoryAssets.find(assetHandle) != m_memoryAssets.end())
			|| GetMetadata(assetHandle).IsValid();
	}

	bool EditorAssetManager::IsAssetLoaded(AssetHandle assetHandle)
	{
		return m_loadedAssets.find(assetHandle) != m_loadedAssets.end();
	}

	bool EditorAssetManager::IsMemoryAsset(AssetHandle assetHandle)
	{
		return m_memoryAssets.find(assetHandle) != m_memoryAssets.end();
	}

	AssetHandle EditorAssetManager::ImportAsset(const std::filesystem::path& filepath)
	{
		auto relativePath = GetRelativePath(filepath);

		// 已存在该路径的资源 → 直接返回
		const auto& existingMetadata = GetMetadata(relativePath);
		if (existingMetadata.IsValid())
			return existingMetadata.handle;

		// 识别资源类型
		AssetType type = GetAssetTypeFromPath(relativePath);
		if (type == AssetType::None)
			return 0;

		// 创建新元数据
		AssetMetadata metadata;
		metadata.handle = AssetHandle(); // 生成新的 UUID
		metadata.filePath = relativePath;
		metadata.type = type;

		m_assetRegistry.Set(metadata.handle, metadata);

		return metadata.handle;
	}

	bool EditorAssetManager::ReloadData(AssetHandle assetHandle)
	{
		const auto& metadata = GetMetadata(assetHandle);
		if (!metadata.IsValid())
		{
			YUICY_CORE_ERROR("[AssetManager] Trying to reload invalid asset");
			return false;
		}

		Ref<Asset> asset = nullptr;
		AssetMetadata updatedMetadata = metadata;
		updatedMetadata.isDataLoaded = AssetImporter::TryLoadData(updatedMetadata, asset);

		if (updatedMetadata.isDataLoaded)
		{
			m_loadedAssets[assetHandle] = asset;
			m_assetRegistry.Set(assetHandle, updatedMetadata);
			YUICY_CORE_INFO("[AssetManager] Reloaded asset: {0}", updatedMetadata.filePath.string());
		}
		else
		{
			YUICY_CORE_ERROR("[AssetManager] Failed to reload asset: {0}", updatedMetadata.filePath.string());
		}

		return updatedMetadata.isDataLoaded;
	}

	void EditorAssetManager::RemoveAsset(AssetHandle assetHandle)
	{
		m_memoryAssets.erase(assetHandle);
		m_loadedAssets.erase(assetHandle);

		if (m_assetRegistry.Contains(assetHandle))
			m_assetRegistry.Remove(assetHandle);
	}

	void EditorAssetManager::AddMemoryOnlyAsset(Ref<Asset> asset)
	{
		m_memoryAssets[asset->handle] = asset;
	}

	AssetHandle EditorAssetManager::GetAssetHandleFromFilePath(const std::filesystem::path& filepath)
	{
		return GetMetadata(filepath).handle;
	}

	AssetType EditorAssetManager::GetAssetTypeFromExtension(const std::string& extension)
	{
		std::string ext = extension;
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (const auto it = s_assetExtensionMap.find(ext); it != s_assetExtensionMap.end())
			return it->second;

		return AssetType::None;
	}

	AssetType EditorAssetManager::GetAssetTypeFromPath(const std::filesystem::path& path)
	{
		return GetAssetTypeFromExtension(path.extension().string());
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(AssetHandle assetHandle)
	{
		return GetFileSystemPath(GetMetadata(assetHandle));
	}

	std::filesystem::path EditorAssetManager::GetFileSystemPath(const AssetMetadata& metadata)
	{
		return Project::GetActiveAssetDirectory() / metadata.filePath;
	}

	std::filesystem::path EditorAssetManager::GetRelativePath(const std::filesystem::path& filepath)
	{
		const auto normalizedFile = filepath.lexically_normal();
		const auto normalizedAssetDir = Project::GetActiveAssetDirectory().lexically_normal();

		// 判断：filepath 是否位于 assetDir 下
		auto fileIt = normalizedFile.begin();
		auto dirIt  = normalizedAssetDir.begin();

		for (; dirIt != normalizedAssetDir.end() && fileIt != normalizedFile.end(); ++dirIt, ++fileIt)
		{
			if (*dirIt != *fileIt)
				return normalizedFile; // 不是子路径，直接返回规范化后的原路径
		}

		// assetDir 还没比较完，说明 filepath 比 assetDir 还短，不可能是其子路径
		if (dirIt != normalizedAssetDir.end())
			return normalizedFile;

		// 走到这里，说明 normalizedFile == normalizedAssetDir
		// 或 normalizedFile 位于 normalizedAssetDir 下面
		auto relativePath = normalizedFile.lexically_relative(normalizedAssetDir);

		// 两者相同，会得到 "."
		// 这里一般不算空；但保留兜底更稳
		return relativePath.empty() ? normalizedFile : relativePath;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(AssetHandle assetHandle) const
	{
		if (m_assetRegistry.Contains(assetHandle))
			return m_assetRegistry.Get(assetHandle);

		return s_nullMetadata;
	}

	const AssetMetadata& EditorAssetManager::GetMetadata(const std::filesystem::path& filepath) const
	{
		const auto relativePath = GetRelativePath(filepath);

		for (const auto& [handle, metadata] : m_assetRegistry)
		{
			if (metadata.filePath == relativePath)
				return metadata;
		}

		return s_nullMetadata;
	}

	std::unordered_set<AssetHandle> EditorAssetManager::GetAllAssetsWithType(AssetType type)
	{
		std::unordered_set<AssetHandle> result;

		// memory-only 资源
		for (const auto& [handle, asset] : m_memoryAssets)
		{
			if (asset->GetAssetType() == type)
				result.insert(handle);
		}

		// 注册表资源
		for (const auto& [handle, metadata] : m_assetRegistry)
		{
			if (metadata.type == type)
				result.insert(handle);
		}

		return result;
	}

	// 注册表持久化
	void EditorAssetManager::LoadAssetRegistry()
	{
		YUICY_CORE_INFO("[AssetManager] Loading Asset Registry");

		auto assetRegistryPath = Project::GetAssetRegistryPath();
		if (!std::filesystem::exists(assetRegistryPath))
			return;

		std::ifstream stream(assetRegistryPath);
		YUICY_CORE_ASSERT(stream);
		std::stringstream strStream;
		strStream << stream.rdbuf();

		YAML::Node data = YAML::Load(strStream.str());
		auto handles = data["Assets"];
		if (!handles)
		{
			YUICY_CORE_ERROR("[AssetManager] Asset Registry appears to be corrupted!");
			return;
		}

		for (auto entry : handles)
		{
			std::string filepath = entry["FilePath"].as<std::string>();

			AssetMetadata metadata;
			metadata.handle = entry["Handle"].as<uint64_t>();
			metadata.filePath = filepath;
			metadata.type = Utils::AssetTypeFromString(entry["Type"].as<std::string>());

			if (metadata.type == AssetType::None)
				continue;

			// 验证扩展名和记录的类型是否匹配
			if (metadata.type != GetAssetTypeFromPath(filepath))
			{
				YUICY_CORE_WARN("[AssetManager] Mismatch between stored AssetType and extension type!");
				metadata.type = GetAssetTypeFromPath(filepath);
			}

			// 检查文件是否依然存在
			if (!std::filesystem::exists(GetFileSystemPath(metadata)))
			{
				YUICY_CORE_WARN("[AssetManager] Missing asset '{0}' detected in registry", metadata.filePath.string());
				continue;
			}

			if (metadata.handle == 0)
			{
				YUICY_CORE_WARN("[AssetManager] AssetHandle for {0} is 0, skipping", metadata.filePath.string());
				continue;
			}

			m_assetRegistry.Set(metadata.handle, metadata);
		}

		YUICY_CORE_INFO("[AssetManager] Loaded {0} asset entries", m_assetRegistry.Count());
	}

	void EditorAssetManager::ProcessDirectory(const std::filesystem::path& directoryPath)
	{
		for (auto entry : std::filesystem::directory_iterator(directoryPath))
		{
			if (entry.is_directory())
				ProcessDirectory(entry.path());
			else
				ImportAsset(entry.path());
		}
	}

	void EditorAssetManager::ReloadAssets()
	{
		ProcessDirectory(Project::GetActiveAssetDirectory().string());
		WriteRegistryToFile();
	}

	void EditorAssetManager::WriteRegistryToFile()
	{
		// 按 UUID 排序以便于项目管理
		struct AssetRegistryEntry
		{
			std::string filePath;
			AssetType type;
		};

		std::map<UUID, AssetRegistryEntry> sortedMap;
		for (auto& [handle, metadata] : m_assetRegistry)
		{
			if (!std::filesystem::exists(GetFileSystemPath(metadata)))
				continue;

			std::string pathToSerialize = metadata.filePath.string();
			std::replace(pathToSerialize.begin(), pathToSerialize.end(), '\\', '/');
			sortedMap[metadata.handle] = { pathToSerialize, metadata.type };
		}

		YUICY_CORE_INFO("[AssetManager] Serializing asset registry with {0} entries", sortedMap.size());

		YAML::Emitter out;
		out << YAML::BeginMap;

		out << YAML::Key << "Assets" << YAML::BeginSeq;
		for (auto& [handle, entry] : sortedMap)
		{
			out << YAML::BeginMap;
			out << YAML::Key << "Handle" << YAML::Value << handle;
			out << YAML::Key << "FilePath" << YAML::Value << entry.filePath;
			out << YAML::Key << "Type" << YAML::Value << Utils::AssetTypeToString(entry.type);
			out << YAML::EndMap;
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		auto assetRegistryPath = Project::GetAssetRegistryPath();
		std::ofstream fout(assetRegistryPath);
		fout << out.c_str();
	}

}
