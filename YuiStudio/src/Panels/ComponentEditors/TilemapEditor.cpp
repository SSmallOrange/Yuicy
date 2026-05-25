#include "pch.h"

#include "TilemapEditor.h"

#include "../../Editor/EditorCommandHistory.h"
#include "../../Editor/EditorDirtyTracker.h"
#include "../../Editor/Commands/PaintTileBatchCommand.h"

#include "Yuicy/Scene/Components.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <optional>
#include <utility>
#include <vector>

namespace Yuicy {

	void TilemapEditor::Draw(TilemapComponent& component, Entity entity, EditorDirtyTracker* dt, EditorCommandHistory* commandHistory)
	{
		if (ImGui::ColorEdit4("Color", glm::value_ptr(component.m_color)))
			if (dt) dt->MarkSceneDirty();

		if (ImGui::DragFloat3("Tile Anchor", glm::value_ptr(component.m_tileAnchor), 0.01f, -10.0f, 10.0f, "%.2f"))
			if (dt) dt->MarkSceneDirty();

		if (ImGui::DragInt("Chunk Size", &component.m_chunkSize, 1.0f, 1, 256))
		{
			component.m_chunkSize = std::max(component.m_chunkSize, 1);
			if (dt) dt->MarkSceneDirty();
		}

		ImGui::Text("Cell Count: %d", (int)component.m_cells.size());

		bool hasCells = !component.m_cells.empty();
		if (!hasCells)
			ImGui::BeginDisabled();

		if (ImGui::Button("Clear All Tiles"))
			ClearAllTiles(component, entity, dt, commandHistory);

		if (!hasCells)
			ImGui::EndDisabled();

		ImGui::BeginDisabled();
		ImGui::Button("Compress Bounds");
		ImGui::EndDisabled();
	}

	void TilemapEditor::ClearAllTiles(TilemapComponent& component, Entity entity, EditorDirtyTracker* dt, EditorCommandHistory* commandHistory)
	{
		if (component.m_cells.empty())
			return;

		if (commandHistory && entity)
		{
			std::vector<TileChange> changes;
			changes.reserve(component.m_cells.size());

			for (const auto& [position, cell] : component.m_cells)
				changes.push_back({ position, cell, std::nullopt });

			commandHistory->ExecuteCommand(CreateScope<PaintTileBatchCommand>(entity.GetScene(), entity.GetUUID(), std::move(changes)));
			return;
		}

		component.ClearAllTiles();
		if (dt) dt->MarkSceneDirty();
	}

}
