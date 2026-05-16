#include "pch.h"

#include "EditorAssetWorkflow.h"
#include "EditorContext.h"
#include "EditorSceneController.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Core/Log.h"

#include "imgui/imgui.h"

#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <system_error>

namespace Yuicy {

	namespace {

		std::string ResolveAssetStem(const std::string& name, const std::string& fallbackName)
		{
			if (name.empty())
				return fallbackName;

			std::filesystem::path namePath(name);
			std::string stem = namePath.stem().string();
			return stem.empty() ? fallbackName : stem;
		}

		std::filesystem::path MakeUniqueAssetPath(const std::filesystem::path& directory, const std::string& name,
			const std::string& extension, const std::string& fallbackName)
		{
			const std::string stem = ResolveAssetStem(name, fallbackName);

			for (uint32_t index = 0;; index++)
			{
				std::string filename = stem;
				if (index > 0)
					filename += " " + std::to_string(index);

				std::filesystem::path filepath = directory / (filename + extension);
				std::error_code ec;
				if (!std::filesystem::exists(filepath, ec) && !ec)
					return filepath;
			}
		}

		bool IsValidAssetDirectory(const std::filesystem::path& directory)
		{
			std::error_code ec;
			if (directory.empty() || !std::filesystem::exists(directory, ec) || ec)
				return false;

			return std::filesystem::is_directory(directory, ec) && !ec;
		}

		bool WriteTextFile(const std::filesystem::path& filepath, const std::string& content)
		{
			std::ofstream file(filepath, std::ios::out | std::ios::trunc);
			if (!file.is_open())
			{
				YUICY_CORE_ERROR("[AssetWorkflow] Failed to create asset file: {}", filepath.string());
				return false;
			}

			file << content;
			file.close();

			if (!file.good())
			{
				std::error_code ec;
				std::filesystem::remove(filepath, ec);
				YUICY_CORE_ERROR("[AssetWorkflow] Failed to write asset file: {}", filepath.string());
				return false;
			}

			return true;
		}

	}

	void EditorAssetWorkflow::OpenAsset(const std::filesystem::path& filepath)
	{
		if (filepath.empty())
			return;

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		AssetType type = assetManager->GetAssetTypeFromPath(filepath);

		switch (type)
		{
		case AssetType::Scene:
			if (m_sceneController)
				m_sceneController->OpenScene(filepath);
			break;

		case AssetType::LuaScript:
		case AssetType::Shader:
			OpenFileExternal(filepath);
			break;

		default:
			OpenFileExternal(filepath);
			break;
		}
	}

	bool EditorAssetWorkflow::CreateFolder(const std::filesystem::path& parentDir, const std::string& name)
	{
		std::filesystem::path newPath = parentDir / name;

		std::error_code ec;
		if (std::filesystem::exists(newPath, ec))
		{
			YUICY_CORE_WARN("[AssetWorkflow] Folder already exists: {}", newPath.string());
			return false;
		}

		if (!std::filesystem::create_directories(newPath, ec) || ec)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to create folder: {}", ec.message());
			return false;
		}

		YUICY_CORE_INFO("[AssetWorkflow] Created folder: {}", newPath.string());
		return true;
	}

	bool EditorAssetWorkflow::CreateSceneFile(const std::filesystem::path& directory, const std::string& name)
	{
		std::string filename = name.empty() ? "NewScene" : name;
		std::filesystem::path filepath = directory / (filename + ".yui");

		std::error_code ec;
		if (std::filesystem::exists(filepath, ec))
		{
			YUICY_CORE_WARN("[AssetWorkflow] Scene file already exists: {}", filepath.string());
			return false;
		}

		std::ofstream file(filepath);
		if (!file.is_open())
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to create scene file: {}", filepath.string());
			return false;
		}

		file << "Scene: " << filename << "\n";
		file << "Entities:\n";
		file.close();

		auto assetManager = Project::GetEditorAssetManager();
		if (assetManager)
			assetManager->ImportAsset(filepath);

		YUICY_CORE_INFO("[AssetWorkflow] Created scene: {}", filepath.string());
		return true;
	}

	bool EditorAssetWorkflow::CreateLuaScriptFile(const std::filesystem::path& directory, const std::string& name)
	{
		std::string filename = name.empty() ? "NewScript" : name;
		std::filesystem::path filepath = directory / (filename + ".lua");

		if (std::error_code ec; std::filesystem::exists(filepath, ec))
		{
			YUICY_CORE_WARN("[AssetWorkflow] Script file already exists: {}", filepath.string());
			return false;
		}

		std::ofstream file(filepath);
		if (!file.is_open())
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to create script file: {}", filepath.string());
			return false;
		}
		// TODO: 直接使用提前写好的脚本模板
		file << "local " << filename << " = {}\n\n";
		file << "function " << filename << ":OnCreate()\n\n";
		file << "end\n\n";
		file << "function " << filename << ":OnUpdate(dt)\n\n";
		file << "end\n\n";
		file << "function " << filename << ":OnDestroy()\n\n";
		file << "end\n\n";
		file << "return " << filename << "\n";
		file.close();

		auto assetManager = Project::GetEditorAssetManager();
		if (assetManager)
			assetManager->ImportAsset(filepath);

		YUICY_CORE_INFO("[AssetWorkflow] Created Lua script: {}", filepath.string());
		return true;
	}

	AssetHandle EditorAssetWorkflow::CreateSpriteFile(const std::filesystem::path& directory, const std::string& name, AssetHandle textureHandle)
	{
		if (!IsValidAssetDirectory(directory))
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Invalid asset directory for Sprite: {}", directory.string());
			return 0;
		}

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Cannot create Sprite without active asset manager.");
			return 0;
		}

		std::filesystem::path filepath = MakeUniqueAssetPath(directory, name, ".ysprite", "New Sprite");

		std::ostringstream content;
		content << "Sprite:\n";
		content << "  TextureHandle: " << (uint64_t)textureHandle << "\n";
		content << "  UVMin: [0.0, 0.0]\n";
		content << "  UVMax: [1.0, 1.0]\n";
		content << "  Pivot: [0.5, 0.5]\n";
		content << "  Border: [0.0, 0.0, 0.0, 0.0]\n";
		content << "  PixelsPerUnit: 100.0\n";

		if (!WriteTextFile(filepath, content.str()))
			return 0;

		AssetHandle handle = assetManager->ImportAsset(filepath);
		if (handle == 0)
		{
			std::error_code ec;
			std::filesystem::remove(filepath, ec);
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to import Sprite asset: {}", filepath.string());
			return 0;
		}

		if (m_context)
		{
			m_context->selection.selectedAsset = handle;
			m_context->selection.ClearEntitySelection();
		}

		YUICY_CORE_INFO("[AssetWorkflow] Created Sprite: {}", filepath.string());
		return handle;
	}

	AssetHandle EditorAssetWorkflow::CreateTileFile(const std::filesystem::path& directory, const std::string& name, AssetHandle spriteHandle)
	{
		if (!IsValidAssetDirectory(directory))
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Invalid asset directory for Tile: {}", directory.string());
			return 0;
		}

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Cannot create Tile without active asset manager.");
			return 0;
		}

		std::filesystem::path filepath = MakeUniqueAssetPath(directory, name, ".ytile", "New Tile");

		std::ostringstream content;
		content << "Tile:\n";
		content << "  SpriteHandle: " << (uint64_t)spriteHandle << "\n";
		content << "  Color: [1.0, 1.0, 1.0, 1.0]\n";
		content << "  ColliderType: None\n";
		content << "  Flags: 0\n";

		if (!WriteTextFile(filepath, content.str()))
			return 0;

		AssetHandle handle = assetManager->ImportAsset(filepath);
		if (handle == 0)
		{
			std::error_code ec;
			std::filesystem::remove(filepath, ec);
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to import Tile asset: {}", filepath.string());
			return 0;
		}

		if (m_context)
		{
			m_context->selection.selectedAsset = handle;
			m_context->selection.ClearEntitySelection();
		}

		YUICY_CORE_INFO("[AssetWorkflow] Created Tile: {}", filepath.string());
		return handle;
	}

	AssetHandle EditorAssetWorkflow::CreateTilePaletteFile(const std::filesystem::path& directory, const std::string& name)
	{
		if (!IsValidAssetDirectory(directory))
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Invalid asset directory for Tile Palette: {}", directory.string());
			return 0;
		}

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Cannot create Tile Palette without active asset manager.");
			return 0;
		}

		std::filesystem::path filepath = MakeUniqueAssetPath(directory, name, ".ypalette", "New Palette");
		std::string paletteName = filepath.stem().string();

		std::ostringstream content;
		content << "TilePalette:\n";
		content << "  Name: " << paletteName << "\n";
		content << "  Layout: Rectangular\n";
		content << "  CellSize: [1.0, 1.0]\n";
		content << "  CellGap: [0.0, 0.0]\n";
		content << "  Cells: []\n";

		if (!WriteTextFile(filepath, content.str()))
			return 0;

		AssetHandle handle = assetManager->ImportAsset(filepath);
		if (handle == 0)
		{
			std::error_code ec;
			std::filesystem::remove(filepath, ec);
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to import Tile Palette asset: {}", filepath.string());
			return 0;
		}

		if (m_context)
		{
			m_context->selection.selectedAsset = handle;
			m_context->selection.ClearEntitySelection();
		}

		YUICY_CORE_INFO("[AssetWorkflow] Created Tile Palette: {}", filepath.string());
		return handle;
	}

	bool EditorAssetWorkflow::CreateSpriteAndTileFromTexture(const std::filesystem::path& texturePath,
		const std::filesystem::path& outputDirectory)
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Cannot create Tile from Texture without active asset manager.");
			return false;
		}

		if (assetManager->GetAssetTypeFromPath(texturePath) != AssetType::Texture)
		{
			YUICY_CORE_WARN("[AssetWorkflow] Path is not a Texture asset: {}", texturePath.string());
			return false;
		}

		AssetHandle textureHandle = assetManager->ImportAsset(texturePath);
		if (textureHandle == 0)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to import source Texture: {}", texturePath.string());
			return false;
		}

		std::filesystem::path targetDirectory = outputDirectory.empty() ? texturePath.parent_path() : outputDirectory;
		std::string baseName = texturePath.stem().string();

		AssetHandle spriteHandle = CreateSpriteFile(targetDirectory, baseName, textureHandle);
		if (spriteHandle == 0)
			return false;

		AssetHandle tileHandle = CreateTileFile(targetDirectory, baseName, spriteHandle);
		if (tileHandle == 0)
		{
			std::filesystem::path spritePath = assetManager->GetFileSystemPath(spriteHandle);
			DeletePath(spritePath);
			return false;
		}

		return true;
	}

	bool EditorAssetWorkflow::DeletePath(const std::filesystem::path& path)
	{
		if (path.empty())
			return false;

		std::error_code ec;
		if (!std::filesystem::exists(path, ec) || ec)
		{
			YUICY_CORE_WARN("[AssetWorkflow] Path does not exist: {}", path.string());
			return false;
		}

		if (std::filesystem::is_regular_file(path, ec))
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
			{
				AssetHandle handle = assetManager->GetAssetHandleFromFilePath(path);
				if (handle != 0)
					assetManager->RemoveAsset(handle);
			}
		}

		std::uintmax_t removed = std::filesystem::remove_all(path, ec);
		if (ec)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to delete: {} ({})", path.string(), ec.message());
			return false;
		}

		YUICY_CORE_INFO("[AssetWorkflow] Deleted: {} ({} items)", path.string(), removed);
		return true;
	}

	bool EditorAssetWorkflow::RenamePath(const std::filesystem::path& oldPath, const std::string& newName)
	{
		if (oldPath.empty() || newName.empty())
			return false;

		std::filesystem::path newPath = oldPath.parent_path() / newName;

		std::error_code ec;
		if (std::filesystem::exists(newPath, ec))
		{
			YUICY_CORE_WARN("[AssetWorkflow] Target name already exists: {}", newPath.string());
			return false;
		}

		std::filesystem::rename(oldPath, newPath, ec);
		if (ec)
		{
			YUICY_CORE_ERROR("[AssetWorkflow] Failed to rename: {}", ec.message());
			return false;
		}

		if (std::filesystem::is_regular_file(newPath, ec))
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
			{
				AssetHandle oldHandle = assetManager->GetAssetHandleFromFilePath(oldPath);
				if (oldHandle != 0)
					assetManager->RemoveAsset(oldHandle);
				assetManager->ImportAsset(newPath);
			}
		}

		YUICY_CORE_INFO("[AssetWorkflow] Renamed: {} -> {}", oldPath.string(), newPath.string());
		return true;
	}

	void EditorAssetWorkflow::CopyPathToClipboard(const std::filesystem::path& path)
	{
		std::string pathStr = path.string();
		ImGui::SetClipboardText(pathStr.c_str());
	}

	void EditorAssetWorkflow::RevealInExplorer(const std::filesystem::path& path)
	{
		if (path.empty())
			return;

		if (std::error_code ec; !std::filesystem::exists(path, ec) || ec)
			return;

		ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}

	void EditorAssetWorkflow::OpenFileExternal(const std::filesystem::path& filepath)
	{
		if (filepath.empty())
			return;

		if (std::error_code ec; !std::filesystem::exists(filepath, ec) || ec)
		{
			YUICY_CORE_WARN("[AssetWorkflow] File does not exist: {}", filepath.string());
			return;
		}

		ShellExecuteW(nullptr, L"open", filepath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	}

}
