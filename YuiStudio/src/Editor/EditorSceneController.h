#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace Yuicy {

	struct EditorContext;
	class EditorDirtyTracker;

	// 编辑器场景/项目生命周期管理器
	class EditorSceneController
	{
	public:
		EditorSceneController() = default;
		~EditorSceneController() = default;

		void SetContext(EditorContext* context) { m_context = context; }
		void SetDirtyTracker(EditorDirtyTracker* tracker) { m_dirtyTracker = tracker; }

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

		// Play / Stop / Simulate
		void OnScenePlay();
		void OnSceneSimulate();
		void OnSceneStop();
		void OnScenePause();
		void OnSceneStep();

		// 场景切换后的回调（用于 EditorLayer 通知面板更新等）
		using SceneChangedCallback = std::function<void()>;
		void SetOnSceneChanged(SceneChangedCallback callback) { m_onSceneChanged = std::move(callback); }

		// 未保存提示 UI
		// TODO: 后续考虑解耦成单独的提示任务类
		void OnImGuiRender();

		// 文件对话框（Win32 平台实现）
		static std::string ShowOpenFileDialog(const char* filter);
		static std::string ShowSaveFileDialog(const char* filter, const char* defaultExtension = nullptr);

	private:
		void NotifySceneChanged();

		// 未保存提示：检查 dirty 并触发对话框
		enum class PendingAction { None, NewScene, OpenScene, OpenProject, NewProject };
		bool CheckDirtyAndConfirm(PendingAction action);
		void ExecutePendingAction();

		EditorContext* m_context = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		SceneChangedCallback m_onSceneChanged;

		// 待执行操作（等待模态框确认）
		PendingAction m_pendingAction = PendingAction::None;
		std::filesystem::path m_pendingFilePath;
		bool m_showUnsavedDialog = false;
	};

}
