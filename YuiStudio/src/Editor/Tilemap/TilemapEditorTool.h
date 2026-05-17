#pragma once

#include "Editor/Commands/PaintTileBatchCommand.h"

#include "Yuicy/Core/MouseCodes.h"
#include "Yuicy/Core/Timestep.h"

#include <optional>
#include <unordered_map>
#include <vector>

namespace Yuicy {

	struct EditorContext;
	struct TilemapComponent;
	enum class TilemapTool;
	class EditorCommandHistory;
	class EditorDirtyTracker;
	class EditorCamera;
	class Entity;
	class Event;
	class KeyPressedEvent;
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
		bool IsPaintStrokeTool() const;
		bool IsBoxFillStrokeTool() const;
		Entity GetActiveTilemapEntity() const;
		Entity GetGridEntity(Entity tilemapEntity) const;
		bool UpdateHoveredCell();
		void BeginStroke(MouseCode mouseButton);
		bool ApplyStrokeAtHoveredCell();
		bool ApplyBoxFillAtHoveredCell();
		bool PickTileAtHoveredCell();
		void UpdateBoxFillPreview();
		void RecordTileChange(Entity tilemapEntity, const GridPosition& position, const std::optional<TileCell>& after);
		void RecordRectangleChanges(Entity tilemapEntity, const GridPosition& startCell, const GridPosition& endCell, const std::optional<TileCell>& after);
		void RemoveStrokeChange(size_t index);
		std::optional<TileCell> GetCellSnapshot(const TilemapComponent& tilemap, const GridPosition& position) const;
		bool TileCellsEqual(const TileCell& lhs, const TileCell& rhs) const;
		bool TileCellOptionalsEqual(const std::optional<TileCell>& lhs, const std::optional<TileCell>& rhs) const;
		void ApplyTileCell(TilemapComponent& tilemap, const GridPosition& position, const std::optional<TileCell>& cell);

		bool OnMouseButtonPressed(MouseButtonPressedEvent& e);
		bool OnMouseButtonReleased(MouseButtonReleasedEvent& e);
		bool OnMouseMoved(MouseMovedEvent& e);
		bool OnKeyPressed(KeyPressedEvent& e);
		void FinishStroke(bool submitCommand);
		void ClearStrokeState();

	private:
		EditorContext* m_context = nullptr;
		EditorCommandHistory* m_commandHistory = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		EditorCamera* m_editorCamera = nullptr;

		bool m_consumingStroke = false;
		UUID m_strokeTilemapEntity = 0;
		TilemapTool m_strokeTool = {};
		AssetHandle m_strokeTile = 0;
		GridPosition m_strokeStartCell;
		MouseCode m_strokeMouseButton = Mouse::ButtonLeft;
		bool m_strokeErases = false;
		std::vector<TileChange> m_strokeChanges;
		std::unordered_map<GridPosition, size_t, GridPositionHash> m_strokeChangeIndices;
	};

}
