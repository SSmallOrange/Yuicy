#include "pch.h"

#include "TilemapRendererEditor.h"

#include "../../Editor/EditorDirtyTracker.h"

#include "Yuicy/Project/Project.h"
#include "Yuicy/Scene/Components.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

namespace Yuicy {

	namespace {

		struct TilemapRenderModeEntry
		{
			TilemapRenderMode mode;
			const char* label;
		};

		constexpr TilemapRenderModeEntry s_renderModeEntries[] = {
			{ TilemapRenderMode::Chunk,      "Chunk" },
			{ TilemapRenderMode::Individual, "Individual" }
		};

	}

	void TilemapRendererEditor::Draw(TilemapRendererComponent& component, EditorDirtyTracker* dt)
	{
		const char* currentMode = TilemapUtils::TilemapRenderModeToString(component.m_mode);
		if (ImGui::BeginCombo("Render Mode", currentMode))
		{
			for (const TilemapRenderModeEntry& entry : s_renderModeEntries)
			{
				bool selected = component.m_mode == entry.mode;
				if (ImGui::Selectable(entry.label, selected))
				{
					component.m_mode = entry.mode;
					if (dt) dt->MarkSceneDirty();
				}

				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		if (auto project = Project::GetActive())
		{
			const auto& sortingLayers = project->GetConfig().SortingLayers;
			if (ImGui::BeginCombo("Sorting Layer", component.m_sortingLayer.c_str()))
			{
				for (const auto& layer : sortingLayers.Layers)
				{
					bool selected = component.m_sortingLayer == layer.Name;
					if (ImGui::Selectable(layer.Name.c_str(), selected))
					{
						component.m_sortingLayer = layer.Name;
						if (dt) dt->MarkSceneDirty();
					}

					if (selected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}
		}
		else
		{
			std::array<char, 128> layerBuffer{};
			std::memcpy(layerBuffer.data(), component.m_sortingLayer.c_str(), std::min(component.m_sortingLayer.size(), layerBuffer.size() - 1));

			if (ImGui::InputText("Sorting Layer", layerBuffer.data(), layerBuffer.size()))
			{
				component.m_sortingLayer = std::string(layerBuffer.data());
				if (dt) dt->MarkSceneDirty();
			}
		}

		if (ImGui::DragInt("Order in Layer", &component.m_sortingOrder))
			if (dt) dt->MarkSceneDirty();
	}

}
