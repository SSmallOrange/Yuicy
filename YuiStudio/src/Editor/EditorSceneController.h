#pragma once

#include <filesystem>
#include <functional>

#include "Yuicy/Core/Base.h"

namespace Yuicy {

	struct EditorContext;
	class EditorDirtyTracker;
	class Scene;

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
		// 不检查场景脏标记，调用方负责先完成未保存确认；打开失败时保持当前项目与场景不变
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

	private:
		void NotifySceneChanged();

		// 新场景交给 EditorContext 之前、以及切换项目之后都必须调用，否则场景拿不到资源与排序层配置
		void AttachSceneContext(const Ref<Scene>& scene) const;

		// 返回已设置当前项目上下文的新场景，内含默认相机与示例 Sprite
		Ref<Scene> CreateDefaultScene() const;
		// 设为编辑场景并清空实体选择、悬停实体与场景脏标记；scenePath 为空表示未保存过的场景
		void SetEditorScene(const Ref<Scene>& scene, const std::filesystem::path& scenePath);

		// 须在构造新会话前调用：新会话从磁盘读取注册表，重新打开同一项目时才能拿到当前会话导入的资源
		void WriteCurrentAssetRegistry() const;

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
