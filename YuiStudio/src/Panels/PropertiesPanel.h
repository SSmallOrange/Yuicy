#pragma once

#include "Yuicy/Core/Base.h"
#include "ComponentEditors/AnimationEditor.h"
#include "ComponentEditors/CameraEditor.h"
#include "ComponentEditors/SpriteEditor.h"

#include <glm/fwd.hpp>
#include <string>

namespace Yuicy {

	class Scene;
	class Entity;
	struct EditorSelectionContext;
	class EditorDirtyTracker;
	class EditorCommandHistory;

	// Properties 面板
	// 负责显示和编辑选中实体的组件
	class PropertiesPanel
	{
	public:
		PropertiesPanel() = default;

		void SetContext(const Ref<Scene>& scene) { m_context = scene; }
		void SetSelectionContext(EditorSelectionContext* selectionContext) { m_editorSelection = selectionContext; }
		void SetDirtyTracker(EditorDirtyTracker* tracker) { m_dirtyTracker = tracker; }
		void SetCommandHistory(EditorCommandHistory* history) { m_commandHistory = history; }

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
		EditorCommandHistory* m_commandHistory = nullptr;

		// Component Editors
		AnimationEditor m_animationEditor;
		CameraEditor m_cameraEditor;
		SpriteEditor m_spriteEditor;
	};

}
