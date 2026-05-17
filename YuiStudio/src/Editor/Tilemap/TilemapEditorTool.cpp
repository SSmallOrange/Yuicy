#include "pch.h"

#include "TilemapEditorTool.h"

#include "Editor/EditorCommandHistory.h"
#include "Editor/EditorContext.h"
#include "Editor/EditorDirtyTracker.h"

#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Core/MouseCodes.h"
#include "Yuicy/Events/Event.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Scene/Components.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Tilemap/GridLayoutUtility.h"

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
		EndStroke();

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
			EndStroke();
			m_context->tilemap.ClearHover();
			return;
		}

		UpdateHoveredCell();

		if (m_consumingStroke && !Input::IsMouseButtonPressed(Mouse::ButtonLeft))
			EndStroke();
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
		if (!m_context)
			return false;

		switch (m_context->tilemap.m_activeTool)
		{
		case TilemapTool::Paint:
		case TilemapTool::Erase:
		case TilemapTool::BoxFill:
		case TilemapTool::FloodFill:
			return true;
		default:
			return false;
		}
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

	bool TilemapEditorTool::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() != Mouse::ButtonLeft)
			return false;

		if (!CanUseActiveTool() || IsCameraNavigationInput())
			return false;

		if (!m_context->tilemap.m_hasHoveredCell && !UpdateHoveredCell())
			return false;

		if (IsStrokeTool())
		{
			m_consumingStroke = true;
			m_context->tilemap.m_isPainting = true;
		}

		return true;
	}

	bool TilemapEditorTool::OnMouseButtonReleased(MouseButtonReleasedEvent& e)
	{
		if (e.GetMouseButton() != Mouse::ButtonLeft || !m_consumingStroke)
			return false;

		EndStroke();
		return true;
	}

	bool TilemapEditorTool::OnMouseMoved(MouseMovedEvent& e)
	{
		(void)e;

		return m_consumingStroke && !IsCameraNavigationInput();
	}

	void TilemapEditorTool::EndStroke()
	{
		m_consumingStroke = false;

		if (m_context)
			m_context->tilemap.EndBrushStroke();
	}

}
