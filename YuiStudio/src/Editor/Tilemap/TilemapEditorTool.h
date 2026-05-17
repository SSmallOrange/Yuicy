#pragma once

#include "Yuicy/Core/Timestep.h"

namespace Yuicy {

	struct EditorContext;
	class EditorCommandHistory;
	class EditorDirtyTracker;
	class EditorCamera;
	class Entity;
	class Event;
	class MouseButtonPressedEvent;
	class MouseButtonReleasedEvent;
	class MouseMovedEvent;

	class TilemapEditorTool
	{
	public:
		TilemapEditorTool() = default;
		~TilemapEditorTool() = default;

		void Init(EditorContext* context, EditorCommandHistory* commandHistory, EditorDirtyTracker* dirtyTracker, EditorCamera* editorCamera);

		void SetContext(EditorContext* context) { m_context = context; }
		void SetCommandHistory(EditorCommandHistory* commandHistory) { m_commandHistory = commandHistory; }
		void SetDirtyTracker(EditorDirtyTracker* dirtyTracker) { m_dirtyTracker = dirtyTracker; }
		void SetEditorCamera(EditorCamera* editorCamera) { m_editorCamera = editorCamera; }

		void OnSceneChanged();
		void OnUpdate(Timestep ts);
		void OnImGuiRender();
		bool OnEvent(Event& e);

		bool IsActive() const;
		bool ShouldBlockViewportEditing() const;

	private:
		bool HasDependencies() const;
		bool IsViewportReady() const;
		bool IsCameraNavigationInput() const;
		bool HasValidActiveTilemap() const;
		bool HasUsableGrid() const;
		bool HasActiveTile() const;
		bool CanUseActiveTool() const;
		bool RequiresActiveTile() const;
		bool IsStrokeTool() const;
		Entity GetActiveTilemapEntity() const;
		Entity GetGridEntity(Entity tilemapEntity) const;
		bool UpdateHoveredCell();

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		void EndStroke();

	private:
		EditorContext* m_context = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		EditorCamera* m_editorCamera = nullptr;

		bool m_consumingStroke = false;
	};

}
