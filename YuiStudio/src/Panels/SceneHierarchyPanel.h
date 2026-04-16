#pragma once

#include "Yuicy.h"
#include "../Editor/EditorSelectionContext.h"

namespace Yuicy {

	// 场景层级面板
	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene);

		void SetContext(const Ref<Scene>& scene);
		void SetSelectionContext(EditorSelectionContext* selectionContext) { m_editorSelection = selectionContext; }

		void OnImGuiRender();

		// 选择操作
		Entity GetSelectedEntity() const;
		void SetSelectedEntity(Entity entity);

	private:
		void DrawEntityNode(Entity entity);

	private:
		Ref<Scene> m_context;
		EditorSelectionContext* m_editorSelection = nullptr;
	};

}
