#pragma once

namespace Yuicy {

	struct SpriteRendererComponent;
	class EditorDirtyTracker;

	class SpriteEditor
	{
	public:
		SpriteEditor() = default;

		void Draw(SpriteRendererComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
