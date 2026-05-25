#pragma once

namespace Yuicy {

	struct TilemapRendererComponent;
	class EditorDirtyTracker;

	class TilemapRendererEditor
	{
	public:
		TilemapRendererEditor() = default;

		void Draw(TilemapRendererComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
