#pragma once

#include "Yuicy.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Panels/SceneHierarchyPanel.h"

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

	private:
		// 场景操作
		void NewScene();
		void OpenScene();
		void SaveScene();
		void SaveSceneAs();

		// 文件对话框
		std::string OpenFileDialog(const char* filter);
		std::string SaveFileDialog(const char* filter);

	private:
		void OnImGuiViewportRender();	// 视口渲染
		void OnImGuiDrawStateRender();  // 渲染信息统计

		// 自定义标题栏
		float UIDrawTitlebar();
	private:
		enum class SceneState { Edit = 0, Play = 1 };

		// 场景
		Ref<Scene> m_editorScene;
		Ref<Scene> m_activeScene;
		SceneState m_sceneState = SceneState::Edit;
		std::filesystem::path m_currentScenePath;

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

		bool m_titleBarHovered = false;
	};

}
