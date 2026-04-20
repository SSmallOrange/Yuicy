#include "EditorAssetWorkflow.h"
#include "EditorSceneController.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Core/Log.h"

#include "imgui/imgui.h"

#include <shellapi.h>
#include <fstream>

namespace Yuicy {

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
