#pragma once

#include "Yuicy.h"
#include "Yuicy/ImGui/ImGuizmo.h"
#include "Yuicy/Renderer/EditorCamera.h"

#include "Panels/ContentBrowserPanel.h"
#include "Panels/SceneHierarchyPanel.h"
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
		void OnScenePlay();
		void OnSceneStop();

	private:
		// 场景操作
		void NewScene();
		void OpenScene();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();
		void SaveSceneAs();

		// 项目操作
		void NewProject();
		void OpenProject();
		void OpenProject(const std::filesystem::path& filepath);
		void SaveProject();

		// 文件对话框
		std::string OpenFileDialog(const char* filter);
		std::string SaveFileDialog(const char* filter, const char* defaultExtension = nullptr);

	private:
		void OnImGuiViewportRender();	// 视口渲染
		void OnImGuiDrawStateRender();  // 渲染信息统计
		void OnImGuiDrawGizmos();       // Gizmo 绘制
		void OnImGuiToolbarRender();    // Play/Stop 工具栏

		// 自定义标题栏
		float UIDrawTitlebar();
	private:
		enum class SceneState { Edit = 0, Play = 1 };

		// 场景
		Ref<Scene> m_editorScene;
		Ref<Scene> m_activeScene;
		SceneState m_sceneState = SceneState::Edit;

		std::filesystem::path m_currentScenePath;
		std::filesystem::path m_currentProjectPath;

		// 渲染
		Ref<Framebuffer> m_framebuffer;
		glm::vec2 m_viewportSize = { 0.0f, 0.0f };
		std::array<glm::vec2, 2> m_viewportBounds = {};

		bool m_viewportFocused = false;
		bool m_viewportHovered = false;

		// 编辑器相机
		EditorCamera m_editorCamera;

		// 鼠标拾取
		Entity m_hoveredEntity;

		// 面板
		SceneHierarchyPanel m_sceneHierarchyPanel;
		ContentBrowserPanel m_contentBrowserPanel;

		bool m_titleBarHovered = false;

		// Gizmo
		int m_gizmoType = -1;

		// 编辑器图标
		Ref<Texture2D> m_playIcon;
		Ref<Texture2D> m_stopIcon;
	};

}
