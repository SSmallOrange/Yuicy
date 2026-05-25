#pragma once

#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	struct TilemapComponent;
	class EditorCommandHistory;
	class EditorDirtyTracker;

	class TilemapEditor
	{
	public:
		TilemapEditor() = default;

		void Draw(TilemapComponent& component, Entity entity, EditorDirtyTracker* dirtyTracker, EditorCommandHistory* commandHistory);

	private:
		void ClearAllTiles(TilemapComponent& component, Entity entity, EditorDirtyTracker* dirtyTracker, EditorCommandHistory* commandHistory);
	};

}
