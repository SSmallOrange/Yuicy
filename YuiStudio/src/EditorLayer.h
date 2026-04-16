#pragma once

#include "Yuicy.h"
#include "Yuicy/ImGui/ImGuizmo.h"
#include "Yuicy/Renderer/EditorCamera.h"

#include "Editor/EditorContext.h"
#include "Editor/EditorSceneController.h"
#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorDirtyTracker.h"

#include "Panels/ContentBrowserPanel.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertiesPanel.h"
#include "Utils/EditorIconUtils.h"

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
		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);

	private:
		void OnImGuiViewportRender();	// 视口渲染
		void OnImGuiDrawStateRender();  // 渲染信息统计
		void OnImGuiDrawGizmos();       // Gizmo 绘制
		void OnImGuiToolbarRender();    // Play/Stop 工具栏

		// 自定义标题栏
		float UIDrawTitlebar();

		// 场景切换后刷新面板
		void OnSceneChanged();

	private:
		// 编辑器核心上下文
		EditorContext m_editorContext;

		// 编辑器场景控制
		EditorSceneController m_sceneController;
		// 操作回溯
		EditorCommandHistory m_commandHistory;
		// 脏数据跟踪
		EditorDirtyTracker m_dirtyTracker;

		// 渲染
		Ref<Framebuffer> m_framebuffer;

		// 编辑器相机
		EditorCamera m_editorCamera;

		// 面板
		SceneHierarchyPanel m_sceneHierarchyPanel;
		PropertiesPanel m_propertiesPanel;
		ContentBrowserPanel m_contentBrowserPanel;

		bool m_titleBarHovered = false;

		// Gizmo
		int m_gizmoType = -1;

		// 编辑器图标
		Ref<Texture2D> m_playIcon;
		Ref<Texture2D> m_stopIcon;
	};

}
