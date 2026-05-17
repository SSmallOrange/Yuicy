#include "pch.h"

#include "TilemapEditorTool.h"

#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorContext.h"
#include "Editor/EditorDirtyTracker.h"

#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Core/MouseCodes.h"
#include "Yuicy/Events/Event.h"
#include "Yuicy/Events/KeyEvent.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Tilemap/GridLayoutUtility.h"

#include <algorithm>

namespace Yuicy {

	void TilemapEditorTool::Init(EditorContext* context, EditorCommandHistory* commandHistory, EditorDirtyTracker* dirtyTracker, EditorCamera* editorCamera)
	{
		m_context = context;
		m_commandHistory = commandHistory;
		m_dirtyTracker = dirtyTracker;
		m_editorCamera = editorCamera;
	}

	void TilemapEditorTool::OnSceneChanged()
	{
		FinishStroke(false);

		if (m_context)
			m_context->tilemap.ClearHover();
	}

	void TilemapEditorTool::OnUpdate(Timestep ts)
	{
		(void)ts;

		if (!m_context)
			return;

		m_context->ValidateTilemapState();

		if (!CanUseActiveTool())
		{
			FinishStroke(true);
			m_context->tilemap.ClearHover();
			return;
		}

		bool hasHoveredCell = UpdateHoveredCell();

		if (m_consumingStroke)
		{
			bool mouseHeld = Input::IsMouseButtonPressed(m_strokeMouseButton);
			if (!mouseHeld || (!m_context->viewport.focused && !m_context->viewport.hovered))
			{
				FinishStroke(true);
				return;
			}

			if (hasHoveredCell && !IsCameraNavigationInput())
			{
				if (m_strokeTool == TilemapTool::BoxFill)
					UpdateBoxFillPreview();
				else
					ApplyStrokeAtHoveredCell();
			}
		}
	}

	void TilemapEditorTool::OnImGuiRender()
	{
		if (!m_context)
			return;

		if (!CanUseActiveTool())
			m_context->tilemap.ClearHover();
	}

	bool TilemapEditorTool::OnEvent(Event& e)
	{
		if (e.Handled)
			return false;

		if (!CanUseActiveTool() && !m_consumingStroke)
			return false;

		EventDispatcher dispatcher(e);

		bool handled = false;
		dispatcher.Dispatch<KeyPressedEvent>([this, &handled](KeyPressedEvent& event)
		{
			handled = OnKeyPressed(event);
			return handled;
		});

		if (handled)
			return true;

		dispatcher.Dispatch<MouseButtonPressedEvent>([this, &handled](MouseButtonPressedEvent& event)
		{
			handled = OnMouseButtonPressed(event);
			return handled;
		});

		if (!handled)
		{
			dispatcher.Dispatch<MouseButtonReleasedEvent>([this, &handled](MouseButtonReleasedEvent& event)
			{
				handled = OnMouseButtonReleased(event);
				return handled;
			});
		}

		if (!handled)
		{
			dispatcher.Dispatch<MouseMovedEvent>([this, &handled](MouseMovedEvent& event)
			{
				handled = OnMouseMoved(event);
				return handled;
			});
		}

		return handled;
	}

	bool TilemapEditorTool::IsActive() const
	{
		return CanUseActiveTool();
	}

	bool TilemapEditorTool::ShouldBlockViewportEditing() const
	{
		return CanUseActiveTool();
	}

	bool TilemapEditorTool::HasDependencies() const
	{
		return m_context && m_commandHistory && m_dirtyTracker && m_editorCamera;
	}

	bool TilemapEditorTool::IsViewportReady() const
	{
		if (!m_context)
			return false;

		const auto& viewportState = m_context->viewport;
		return viewportState.hovered || viewportState.focused || m_consumingStroke;
	}

	bool TilemapEditorTool::IsCameraNavigationInput() const
	{
		bool altPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
		bool middleMousePressed = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);
		bool cameraPanning = m_editorCamera && m_editorCamera->IsPanning();
		return altPressed || middleMousePressed || cameraPanning;
	}

	bool TilemapEditorTool::HasValidActiveTilemap() const
	{
		return static_cast<bool>(GetActiveTilemapEntity());
	}

	bool TilemapEditorTool::HasActiveTile() const
	{
		return m_context && m_context->tilemap.m_activeTile != 0;
	}

	bool TilemapEditorTool::CanUseActiveTool() const
	{
		if (!HasDependencies() || !m_context->runtime.IsEditing() || !IsViewportReady())
			return false;

		if (m_context->tilemap.m_activeTool == TilemapTool::Select)
			return false;

		if (!HasValidActiveTilemap())
			return false;

		if (!HasUsableGrid())
			return false;

		if (RequiresActiveTile() && !HasActiveTile())
			return false;

		return true;
	}

	bool TilemapEditorTool::RequiresActiveTile() const
	{
		if (!m_context)
			return false;

		switch (m_context->tilemap.m_activeTool)
		{
		case TilemapTool::Paint:
		case TilemapTool::BoxFill:
		case TilemapTool::FloodFill:
			return true;
		default:
			return false;
		}
	}

	bool TilemapEditorTool::IsStrokeTool() const
	{
		return IsPaintStrokeTool() || IsBoxFillStrokeTool();
	}

	bool TilemapEditorTool::IsPaintStrokeTool() const
	{
		if (!m_context)
			return false;

		switch (m_context->tilemap.m_activeTool)
		{
		case TilemapTool::Paint:
		case TilemapTool::Erase:
			return true;
		default:
			return false;
		}
	}

	bool TilemapEditorTool::IsBoxFillStrokeTool() const
	{
		return m_context && m_context->tilemap.m_activeTool == TilemapTool::BoxFill;
	}

	Entity TilemapEditorTool::GetActiveTilemapEntity() const
	{
		if (!m_context || !m_context->activeScene || m_context->tilemap.m_activeTilemapEntity == 0)
			return {};

		Entity tilemapEntity = m_context->activeScene->FindEntityByUUID(m_context->tilemap.m_activeTilemapEntity);
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return {};

		return tilemapEntity;
	}

	bool TilemapEditorTool::HasUsableGrid() const
	{
		Entity tilemapEntity = GetActiveTilemapEntity();
		Entity gridEntity = GetGridEntity(tilemapEntity);
		if (!gridEntity)
			return false;

		const GridComponent& grid = gridEntity.GetComponent<GridComponent>();
		return grid.m_layout == GridLayout::Rectangular;
	}

	Entity TilemapEditorTool::GetGridEntity(Entity tilemapEntity) const
	{
		if (!tilemapEntity)
			return {};

		if (tilemapEntity.HasComponent<GridComponent>())
			return tilemapEntity;

		Entity parent = tilemapEntity.GetParent();
		if (parent && parent.HasComponent<GridComponent>())
			return parent;

		return {};
	}

	bool TilemapEditorTool::UpdateHoveredCell()
	{
		if (!m_context || !m_editorCamera || !m_context->viewport.hovered)
		{
			if (m_context)
				m_context->tilemap.ClearHover();
			return false;
		}

		Entity tilemapEntity = GetActiveTilemapEntity();
		Entity gridEntity = GetGridEntity(tilemapEntity);
		if (!gridEntity)
		{
			m_context->tilemap.ClearHover();
			return false;
		}

		const GridComponent& grid = gridEntity.GetComponent<GridComponent>();
		if (grid.m_layout != GridLayout::Rectangular)
		{
			m_context->tilemap.ClearHover();
			return false;
		}

		auto [mouseX, mouseY] = ImGui::GetMousePos();
		glm::vec2 viewportMouse = {
			mouseX - m_context->viewport.bounds[0].x,
			mouseY - m_context->viewport.bounds[0].y
		};

		if (viewportMouse.x < 0.0f || viewportMouse.y < 0.0f
			|| viewportMouse.x >= m_context->viewport.size.x
			|| viewportMouse.y >= m_context->viewport.size.y)
		{
			m_context->tilemap.ClearHover();
			return false;
		}

		glm::vec2 world = m_editorCamera->ScreenToWorld(viewportMouse.x, viewportMouse.y);
		glm::mat4 gridWorldTransform = m_context->activeScene->GetWorldSpaceTransformMatrix(gridEntity);
		m_context->tilemap.m_hoveredCell = GridLayoutUtility::WorldToCell(world, gridWorldTransform, grid);
		m_context->tilemap.m_hasHoveredCell = true;
		return true;
	}

	void TilemapEditorTool::BeginStroke(MouseCode mouseButton)
	{
		ClearStrokeState();

		m_consumingStroke = true;
		m_strokeTilemapEntity = m_context->tilemap.m_activeTilemapEntity;
		m_strokeTool = m_context->tilemap.m_activeTool;
		m_strokeTile = m_context->tilemap.m_activeTile;
		m_strokeStartCell = m_context->tilemap.m_hoveredCell;
		m_strokeMouseButton = mouseButton;
		m_strokeErases = m_strokeTool == TilemapTool::Erase || mouseButton == Mouse::ButtonRight;
		m_context->tilemap.m_isPainting = true;

		if (m_strokeTool == TilemapTool::BoxFill)
			UpdateBoxFillPreview();
	}

	bool TilemapEditorTool::ApplyStrokeAtHoveredCell()
	{
		if (!m_context || !m_context->activeScene || !m_context->tilemap.m_hasHoveredCell)
			return false;

		Entity tilemapEntity = m_context->activeScene->FindEntityByUUID(m_strokeTilemapEntity);
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return false;

		std::optional<TileCell> after;
		if (m_strokeErases)
		{
			after = std::nullopt;
		}
		else if (m_strokeTool == TilemapTool::Paint)
		{
			if (m_strokeTile == 0)
				return false;

			TileCell cell;
			cell.m_tileHandle = m_strokeTile;
			after = cell;
		}
		else if (m_strokeTool == TilemapTool::Erase)
		{
			after = std::nullopt;
		}
		else
		{
			return false;
		}

		RecordTileChange(tilemapEntity, m_context->tilemap.m_hoveredCell, after);
		return true;
	}

	bool TilemapEditorTool::ApplyBoxFillAtHoveredCell()
	{
		if (!m_context || !m_context->activeScene)
			return false;

		Entity tilemapEntity = m_context->activeScene->FindEntityByUUID(m_strokeTilemapEntity);
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return false;

		std::optional<TileCell> after;
		if (!m_strokeErases)
		{
			if (m_strokeTile == 0)
				return false;

			TileCell cell;
			cell.m_tileHandle = m_strokeTile;
			after = cell;
		}

		GridPosition endCell = m_context->tilemap.m_hasBoxFillPreview
			? m_context->tilemap.m_boxFillEndCell
			: m_strokeStartCell;

		RecordRectangleChanges(tilemapEntity, m_strokeStartCell, endCell, after);
		return true;
	}

	bool TilemapEditorTool::PickTileAtHoveredCell()
	{
		if (!m_context || !m_context->activeScene || !m_context->tilemap.m_hasHoveredCell)
			return false;

		Entity tilemapEntity = GetActiveTilemapEntity();
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return false;

		const auto& tilemap = tilemapEntity.GetComponent<TilemapComponent>();
		if (const TileCell* cell = tilemap.GetTile(m_context->tilemap.m_hoveredCell))
			m_context->tilemap.m_activeTile = cell->m_tileHandle;
		else
			m_context->tilemap.m_activeTile = 0;

		return true;
	}

	void TilemapEditorTool::UpdateBoxFillPreview()
	{
		if (!m_context || m_strokeTool != TilemapTool::BoxFill || !m_context->tilemap.m_hasHoveredCell)
			return;

		m_context->tilemap.SetBoxFillPreview(m_strokeStartCell, m_context->tilemap.m_hoveredCell, m_strokeErases);
	}

	void TilemapEditorTool::RecordTileChange(Entity tilemapEntity, const GridPosition& position, const std::optional<TileCell>& after)
	{
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return;

		auto& tilemap = tilemapEntity.GetComponent<TilemapComponent>();
		auto it = m_strokeChangeIndices.find(position);
		if (it == m_strokeChangeIndices.end())
		{
			std::optional<TileCell> before = GetCellSnapshot(tilemap, position);
			if (TileCellOptionalsEqual(before, after))
				return;

			size_t index = m_strokeChanges.size();
			m_strokeChanges.push_back({ position, before, after });
			m_strokeChangeIndices[position] = index;
			ApplyTileCell(tilemap, position, after);
			return;
		}

		size_t index = it->second;
		TileChange& change = m_strokeChanges[index];
		change.after = after;
		ApplyTileCell(tilemap, position, after);

		if (TileCellOptionalsEqual(change.before, change.after))
			RemoveStrokeChange(index);
	}

	void TilemapEditorTool::RecordRectangleChanges(Entity tilemapEntity, const GridPosition& startCell, const GridPosition& endCell, const std::optional<TileCell>& after)
	{
		int minX = std::min(startCell.m_x, endCell.m_x);
		int maxX = std::max(startCell.m_x, endCell.m_x);
		int minY = std::min(startCell.m_y, endCell.m_y);
		int maxY = std::max(startCell.m_y, endCell.m_y);
		int minZ = std::min(startCell.m_z, endCell.m_z);
		int maxZ = std::max(startCell.m_z, endCell.m_z);

		for (int z = minZ; z <= maxZ; z++)
		{
			for (int y = minY; y <= maxY; y++)
			{
				for (int x = minX; x <= maxX; x++)
					RecordTileChange(tilemapEntity, { x, y, z }, after);
			}
		}
	}

	void TilemapEditorTool::RemoveStrokeChange(size_t index)
	{
		if (index >= m_strokeChanges.size())
			return;

		m_strokeChangeIndices.erase(m_strokeChanges[index].position);

		size_t lastIndex = m_strokeChanges.size() - 1;
		if (index != lastIndex)
		{
			m_strokeChanges[index] = m_strokeChanges[lastIndex];
			m_strokeChangeIndices[m_strokeChanges[index].position] = index;
		}

		m_strokeChanges.pop_back();
	}

	std::optional<TileCell> TilemapEditorTool::GetCellSnapshot(const TilemapComponent& tilemap, const GridPosition& position) const
	{
		if (const TileCell* cell = tilemap.GetTile(position))
			return *cell;

		return std::nullopt;
	}

	bool TilemapEditorTool::TileCellsEqual(const TileCell& lhs, const TileCell& rhs) const
	{
		return lhs.m_tileHandle == rhs.m_tileHandle
			&& lhs.m_transformFlags == rhs.m_transformFlags
			&& lhs.m_color.r == rhs.m_color.r
			&& lhs.m_color.g == rhs.m_color.g
			&& lhs.m_color.b == rhs.m_color.b
			&& lhs.m_color.a == rhs.m_color.a;
	}

	bool TilemapEditorTool::TileCellOptionalsEqual(const std::optional<TileCell>& lhs, const std::optional<TileCell>& rhs) const
	{
		if (lhs.has_value() != rhs.has_value())
			return false;

		if (!lhs && !rhs)
			return true;

		return TileCellsEqual(*lhs, *rhs);
	}

	void TilemapEditorTool::ApplyTileCell(TilemapComponent& tilemap, const GridPosition& position, const std::optional<TileCell>& cell)
	{
		if (cell)
			tilemap.SetTile(position, *cell);
		else
			tilemap.EraseTile(position);
	}

	bool TilemapEditorTool::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		MouseCode mouseButton = e.GetMouseButton();
		if (mouseButton != Mouse::ButtonLeft && mouseButton != Mouse::ButtonRight)
			return false;

		if (!CanUseActiveTool() || IsCameraNavigationInput())
			return false;

		if (!m_context->tilemap.m_hasHoveredCell && !UpdateHoveredCell())
			return false;

		if (m_context->tilemap.m_activeTool == TilemapTool::Picker)
		{
			if (mouseButton != Mouse::ButtonLeft)
				return false;

			return PickTileAtHoveredCell();
		}

		if (IsStrokeTool())
		{
			BeginStroke(mouseButton);

			if (m_strokeTool == TilemapTool::BoxFill)
				UpdateBoxFillPreview();
			else
				ApplyStrokeAtHoveredCell();
		}

		return true;
	}

	bool TilemapEditorTool::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		if (!m_consumingStroke || e.GetMouseButton() != m_strokeMouseButton)
			return false;

		FinishStroke(true);
		return true;
	}

	bool TilemapEditorTool::OnMouseMoved(MouseMovedEvent& e)
	{
		(void)e;

		if (!m_consumingStroke || IsCameraNavigationInput())
			return false;

		if (UpdateHoveredCell())
		{
			if (m_strokeTool == TilemapTool::BoxFill)
				UpdateBoxFillPreview();
			else
				ApplyStrokeAtHoveredCell();
		}

		return true;
	}

	bool TilemapEditorTool::OnKeyPressed(KeyPressedEvent& e)
	{
		if (!m_consumingStroke || e.IsRepeat() || e.GetKeyCode() != Key::Escape)
			return false;

		FinishStroke(true);
		return true;
	}

	void TilemapEditorTool::FinishStroke(bool submitCommand)
	{
		if (!m_consumingStroke)
		{
			ClearStrokeState();
			return;
		}

		Scene* scene = m_context ? m_context->activeScene.get() : nullptr;
		UUID tilemapEntityUUID = m_strokeTilemapEntity;

		if (submitCommand && m_strokeTool == TilemapTool::BoxFill)
			ApplyBoxFillAtHoveredCell();

		std::vector<TileChange> changes = std::move(m_strokeChanges);

		ClearStrokeState();

		if (!submitCommand || changes.empty() || !scene || tilemapEntityUUID == 0)
			return;

		if (m_commandHistory)
		{
			m_commandHistory->ExecuteCommand(CreateScope<PaintTileBatchCommand>(scene, tilemapEntityUUID, std::move(changes)));
		}
		else if (m_dirtyTracker)
		{
			m_dirtyTracker->MarkSceneDirty();
		}
	}

	void TilemapEditorTool::ClearStrokeState()
	{
		m_consumingStroke = false;
		m_strokeTilemapEntity = 0;
		m_strokeTool = TilemapTool::Select;
		m_strokeTile = 0;
		m_strokeStartCell = {};
		m_strokeMouseButton = Mouse::ButtonLeft;
		m_strokeErases = false;
		m_strokeChanges.clear();
		m_strokeChangeIndices.clear();
		if (m_context)
			m_context->tilemap.EndBrushStroke();
	}

}
