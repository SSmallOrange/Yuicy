#pragma once

namespace Yuicy {

	struct CameraComponent;
	class EditorDirtyTracker;

	class CameraEditor
	{
	public:
		CameraEditor() = default;

		void Draw(CameraComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
