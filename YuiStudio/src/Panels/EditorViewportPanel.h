#pragma once

#include "Yuicy.h"
#include "Yuicy/ImGui/ImGuizmo.h"
#include "Yuicy/Renderer/EditorCamera.h"

#include "../Editor/EditorContext.h"
#include "../Editor/EditorRenderPipeline.h"
#include "../Editor/EditorSceneController.h"
#include "../Editor/EditorDirtyTracker.h"
#include "../Editor/EditorCommandHistory.h"
#include "../Editor/Commands/SetTransformCommand.h"

namespace Yuicy {

	// 编辑器视口面板
	// 管理 Viewport UI、Framebuffer 尺寸同步、拖拽接收、
	// 鼠标拾取坐标换算、Gizmo 绘制和编辑器相机
	class EditorViewportPanel
	{
	public:
		EditorViewportPanel() = default;
		~EditorViewportPanel() = default;

		void Init();

		void SetContext(EditorContext* context) { m_context = context; }
		void SetRenderPipeline(EditorRenderPipeline* pipeline) { m_renderPipeline = pipeline; }
		void SetSceneController(EditorSceneController* controller) { m_sceneController = controller; }
		void SetDirtyTracker(EditorDirtyTracker* tracker) { m_dirtyTracker = tracker; }
		void SetCommandHistory(EditorCommandHistory* history) { m_commandHistory = history; }

		void OnUpdate(Timestep ts);
		void OnImGuiRender();
		void OnEvent(Event& e);

		// 场景切换通知
		void OnSceneChanged();

		EditorCamera& GetEditorCamera() { return m_editorCamera; }

		bool IsGizmoInUse() const { return m_gizmoInUse; }

	private:
		void OnImGuiDrawGizmos();
		void OnImGuiToolbarRender();
		void OnImGuiOverlaySettingsRender();

		void DrawOverlaySettingsPopup();
		void UpdateMousePicking();

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);

		EditorContext* m_context = nullptr;
		EditorRenderPipeline* m_renderPipeline = nullptr;
		EditorSceneController* m_sceneController = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;

		// Gizmo 拖拽快照（拖拽开始时记录初始 Transform）
		bool m_gizmoSnapshotCaptured = false;
		TransformComponent m_gizmoOldTransform;

		// 编辑器相机
		EditorCamera m_editorCamera;

		// Gizmo
		int m_gizmoType = -1;
		bool m_gizmoInUse = false;

		// 编辑器图标
		Ref<Texture2D> m_playIcon;
		Ref<Texture2D> m_simulateIcon;
		Ref<Texture2D> m_stopIcon;
		Ref<Texture2D> m_pauseStartIcon;
		Ref<Texture2D> m_pauseStopIcon;
		Ref<Texture2D> m_stepIcon;
		Ref<Texture2D> m_overlayIcon;
	};

}
