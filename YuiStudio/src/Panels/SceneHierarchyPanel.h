#pragma once

#include "Yuicy/Core/Base.h"

namespace Yuicy {

	class Scene;
	class Entity;
	struct EditorSelectionContext;
	class EditorDirtyTracker;
	class EditorCommandHistory;

	// 场景层级面板
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetSelectionContext(EditorSelectionContext* selectionContext) { m_editorSelection = selectionContext; }
		void SetDirtyTracker(EditorDirtyTracker* tracker) { m_dirtyTracker = tracker; }
		void SetCommandHistory(EditorCommandHistory* history) { m_commandHistory = history; }

		void OnImGuiRender();

		// 选择操作
		Entity GetSelectedEntity() const;
		void SetSelectedEntity(Entity entity);

	private:
		void DrawEntityNode(Entity entity);

	private:
		Ref<Scene> m_context;
		EditorSelectionContext* m_editorSelection = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;
	};

}
