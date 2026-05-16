#pragma once

#include "Yuicy/Asset/Asset.h"

#include <filesystem>
#include <string>

namespace Yuicy {

	struct EditorContext;
	class EditorSceneController;

	// 编辑器资源工作流服务
	// 封装文件系统操作，根据资源类型决定打开行为
	// 委托 ContentBrowserPanel 的文件操作
	class EditorAssetWorkflow
	{
	public:
		EditorAssetWorkflow() = default;
		~EditorAssetWorkflow() = default;

		void SetContext(EditorContext* context) { m_context = context; }
		void SetSceneController(EditorSceneController* controller) { m_sceneController = controller; }

		// 根据资源类型分发打开行为
		void OpenAsset(const std::filesystem::path& filepath);

		// 文件系统操作
		bool CreateFolder(const std::filesystem::path& parentDir, const std::string& name);
		bool CreateSceneFile(const std::filesystem::path& directory, const std::string& name);
		bool CreateLuaScriptFile(const std::filesystem::path& directory, const std::string& name);
		AssetHandle CreateSpriteFile(const std::filesystem::path& directory, const std::string& name, AssetHandle textureHandle = 0);
		AssetHandle CreateTileFile(const std::filesystem::path& directory, const std::string& name, AssetHandle spriteHandle = 0);
		AssetHandle CreateTilePaletteFile(const std::filesystem::path& directory, const std::string& name);
		bool CreateSpriteAndTileFromTexture(const std::filesystem::path& texturePath, const std::filesystem::path& outputDirectory);
		bool DeletePath(const std::filesystem::path& path);
		bool RenamePath(const std::filesystem::path& oldPath, const std::string& newName);
		void CopyPathToClipboard(const std::filesystem::path& path);

		// 系统集成
		void RevealInExplorer(const std::filesystem::path& path);
		void OpenFileExternal(const std::filesystem::path& filepath);

	private:
		EditorContext* m_context = nullptr;
		EditorSceneController* m_sceneController = nullptr;
	};

}
