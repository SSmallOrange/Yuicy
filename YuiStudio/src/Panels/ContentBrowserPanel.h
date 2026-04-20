#pragma once

#include "Yuicy.h"
#include "Yuicy/Asset/AssetTypes.h"

#include <filesystem>

namespace Yuicy {

	struct EditorContext;
	class EditorAssetWorkflow;

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void SetContext(EditorContext* context) { m_context = context; }
		void SetAssetWorkflow(EditorAssetWorkflow* workflow) { m_assetWorkflow = workflow; }

		void OnImGuiRender();

		// 项目/场景切换后重置导航状态
		void ResetNavigation();

	private:
		void DrawNavigationBar();
		void DrawBreadcrumbs();
		bool PassesFilter(const std::filesystem::directory_entry& entry) const;

	private:
		EditorContext* m_context = nullptr;
		EditorAssetWorkflow* m_assetWorkflow = nullptr;

		std::filesystem::path m_baseDirectory;
		std::filesystem::path m_currentDirectory;
		std::filesystem::path m_selectedPath;

		// 搜索与过滤
		char m_searchBuffer[256] = {};
		AssetType m_filterType = AssetType::None;

		// 重命名状态
		std::filesystem::path m_renamingPath;
		char m_renameBuffer[256] = {};
		bool m_renameNeedsFocus = false;
		bool m_renameWasActive = false;

		// 删除确认状态
		std::filesystem::path m_pendingDeletePath;
		bool m_showDeleteConfirmDialog = false;

		Ref<Texture2D> m_directoryIcon;
		Ref<Texture2D> m_fileIcon;
		Ref<Texture2D> m_backIcon;
		Ref<Texture2D> m_clearIcon;

		float m_thumbnailSize = 96.0f;
		float m_padding = 16.0f;
	};

}
