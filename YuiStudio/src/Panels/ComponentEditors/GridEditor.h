#pragma once

namespace Yuicy {

	struct GridComponent;
	class EditorDirtyTracker;

	class GridEditor
	{
	public:
		GridEditor() = default;

		void Draw(GridComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
