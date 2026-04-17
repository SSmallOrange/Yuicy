#pragma once

#include "Yuicy.h"

#include "Editor/EditorContext.h"
#include "Editor/EditorSceneController.h"
#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorDirtyTracker.h"
#include "Editor/EditorRenderPipeline.h"
#include "Editor/EditorOverlayRenderer.h"

#include "Panels/EditorViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"

namespace Yuicy {

	class EditorLayer : public Layer
	{
	public:
		EditorLayer();
		virtual ~EditorLayer() = default;

		void OnAttach() override;
		void OnDetach() override;
		void OnUpdate(Timestep ts) override;
		void OnImGuiRender() override;
		void OnEvent(Event& e) override;

	private:
		bool OnKeyPressed(KeyPressedEvent& e);

	private:
		void OnImGuiDrawStateRender();  // 渲染信息统计

		// 自定义标题栏
		float UIDrawTitlebar();

		// 场景切换后刷新面板
		void OnSceneChanged();

		// 窗口关闭拦截
		void HandleWindowClose();

	private:
		// 编辑器核心上下文
		EditorContext m_editorContext;

		// 编辑器场景控制
		EditorSceneController m_sceneController;
		// 操作回溯
		EditorCommandHistory m_commandHistory;
		// 脏数据跟踪
		EditorDirtyTracker m_dirtyTracker;

		// 渲染管线
		EditorRenderPipeline m_renderPipeline;
		// Overlay
		EditorOverlayRenderer m_overlayRenderer;

		// 面板
		EditorViewportPanel m_viewportPanel;
		SceneHierarchyPanel m_sceneHierarchyPanel;
		PropertiesPanel m_propertiesPanel;
		ContentBrowserPanel m_contentBrowserPanel;

		bool m_titleBarHovered = false;

		// 窗口关闭控制
		bool m_showCloseConfirmDialog = false;
		bool m_pendingClose = false;
	};

}
