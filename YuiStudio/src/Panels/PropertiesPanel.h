#pragma once

#include "Yuicy.h"
#include "../Editor/EditorSelectionContext.h"
#include "../Editor/EditorDirtyTracker.h"

namespace Yuicy {

	// Properties 面板
	// 负责显示和编辑选中实体的组件
	class PropertiesPanel
	{
	public:
		PropertiesPanel() = default;

		void SetContext(const Ref<Scene>& scene) { m_context = scene; }
		void SetSelectionContext(EditorSelectionContext* selectionContext) { m_editorSelection = selectionContext; }
		void SetDirtyTracker(EditorDirtyTracker* tracker) { m_dirtyTracker = tracker; }

		void OnImGuiRender();

	private:
		Entity GetSelectedEntity() const;

		void DrawComponents(Entity entity);

		void DrawVec3Control(const std::string& label, glm::vec3& values,
			float resetValue = 0.0f, float columnWidth = 100.0f);

		template<typename T>
		void DrawAddComponentEntry(const std::string& entryName);

	private:
		Ref<Scene> m_context;
		EditorSelectionContext* m_editorSelection = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
	};

}
