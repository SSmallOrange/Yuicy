#pragma once

#include "Yuicy/Scene/Components.h"
#include "../../Editor/EditorDirtyTracker.h"

namespace Yuicy {

	class CameraEditor
	{
	public:
		CameraEditor() = default;

		void Draw(CameraComponent& component, EditorDirtyTracker* dirtyTracker);
	};

}
