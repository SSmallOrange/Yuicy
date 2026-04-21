#pragma once

#include "Yuicy/Scene/Components.h"
#include "../../Editor/EditorDirtyTracker.h"

namespace Yuicy {

	class ColliderEditor
	{
	public:
		ColliderEditor() = default;

		void DrawBoxCollider(BoxCollider2DComponent& component, EditorDirtyTracker* dirtyTracker);
		void DrawCircleCollider(CircleCollider2DComponent& component, EditorDirtyTracker* dirtyTracker);

	private:
		// 碰撞过滤位掩码编辑（Category / Mask）
		void DrawCollisionFilter(uint16_t& categoryBits, uint16_t& maskBits, EditorDirtyTracker* dirtyTracker);
	};

}
