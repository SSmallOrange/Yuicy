#pragma once

#include "Yuicy/Scene/Components.h"
#include "../../Editor/EditorDirtyTracker.h"

namespace Yuicy {

	class SpriteEditor
	{
	public:
		SpriteEditor() = default;

		void Draw(SpriteRendererComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
