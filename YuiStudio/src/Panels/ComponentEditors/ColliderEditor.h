#pragma once

#include "Yuicy/Scene/Components.h"
#include "../../Editor/EditorDirtyTracker.h"

namespace Yuicy {

	struct EditorContext;

	class ColliderEditor
	{
	public:
		ColliderEditor() = default;

		void SetEditorContext(const EditorContext* context) { m_editorContext = context; }

		void DrawBoxCollider(BoxCollider2DComponent& component, EditorDirtyTracker* dirtyTracker);
		void DrawCircleCollider(CircleCollider2DComponent& component, EditorDirtyTracker* dirtyTracker);

	private:
		// 碰撞过滤位掩码编辑（Category / Mask）；没有打开项目时不显示
		void DrawCollisionFilter(uint16_t& categoryBits, uint16_t& maskBits, EditorDirtyTracker* dirtyTracker);

	private:
		const EditorContext* m_editorContext = nullptr;
	};

}
