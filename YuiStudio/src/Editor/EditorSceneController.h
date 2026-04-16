#pragma once

#include "EditorContext.h"
#include "Yuicy/Renderer/EditorCamera.h"

#include <string>
#include <functional>

namespace Yuicy {

	class SceneHierarchyPanel;

	// 编辑器场景/项目生命周期管理器
	class EditorSceneController
	{
	public:
		EditorSceneController() = default;
		~EditorSceneController() = default;

		void SetContext(EditorContext* context) { m_context = context; }

		// 场景操作
		void NewScene();
		void OpenSceneDialog();
		bool OpenScene(const std::filesystem::path& filepath);
		void SaveScene();
		void SaveSceneAs();

		// 项目操作
		void NewProject();
		void OpenProjectDialog();
		void OpenProject(const std::filesystem::path& filepath);
		void SaveProject();

		// Play / Stop
		void OnScenePlay();
		void OnSceneStop();

		// 场景切换后的回调（用于 EditorLayer 通知面板更新等）
		using SceneChangedCallback = std::function<void()>;
		void SetOnSceneChanged(SceneChangedCallback callback) { m_onSceneChanged = std::move(callback); }

		// 文件对话框（Win32 平台实现）
		static std::string ShowOpenFileDialog(const char* filter);
		static std::string ShowSaveFileDialog(const char* filter, const char* defaultExtension = nullptr);

	private:
		void NotifySceneChanged();

		EditorContext* m_context = nullptr;
		SceneChangedCallback m_onSceneChanged;
	};

}
