#include "pch.h"

#include "GridEditor.h"

#include "../../Editor/EditorDirtyTracker.h"

#include "Yuicy/Scene/Components.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	namespace {

		struct GridLayoutEntry
		{
			GridLayout layout;
			const char* label;
		};

		struct GridCellSwizzleEntry
		{
			GridCellSwizzle swizzle;
			const char* label;
		};

		constexpr GridLayoutEntry s_gridLayoutEntries[] = {
			{ GridLayout::Rectangular, "Rectangular" },
			{ GridLayout::Isometric,   "Isometric" },
			{ GridLayout::Hexagonal,   "Hexagonal" }
		};

		constexpr GridCellSwizzleEntry s_cellSwizzleEntries[] = {
			{ GridCellSwizzle::XYZ, "XYZ" },
			{ GridCellSwizzle::XZY, "XZY" },
			{ GridCellSwizzle::YXZ, "YXZ" },
			{ GridCellSwizzle::YZX, "YZX" },
			{ GridCellSwizzle::ZXY, "ZXY" },
			{ GridCellSwizzle::ZYX, "ZYX" }
		};

	}

	void GridEditor::Draw(GridComponent& component, EditorDirtyTracker* dt)
	{
		const char* currentLayout = TilemapUtils::GridLayoutToString(component.m_layout);
		if (ImGui::BeginCombo("Layout", currentLayout))
		{
			for (const GridLayoutEntry& entry : s_gridLayoutEntries)
			{
				bool selected = component.m_layout == entry.layout;
				if (ImGui::Selectable(entry.label, selected))
				{
					component.m_layout = entry.layout;
					if (dt) dt->MarkSceneDirty();
				}

				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		if (ImGui::DragFloat2("Cell Size", glm::value_ptr(component.m_cellSize), 0.05f, 0.01f, 100.0f, "%.2f"))
		{
			component.m_cellSize.x = std::max(component.m_cellSize.x, 0.01f);
			component.m_cellSize.y = std::max(component.m_cellSize.y, 0.01f);

			component.m_cellGap.x = std::max(component.m_cellGap.x, -component.m_cellSize.x + 0.01f);
			component.m_cellGap.y = std::max(component.m_cellGap.y, -component.m_cellSize.y + 0.01f);

			if (dt) dt->MarkSceneDirty();
		}

		if (ImGui::DragFloat2("Cell Gap", glm::value_ptr(component.m_cellGap), 0.01f, -100.0f, 100.0f, "%.2f"))
		{
			component.m_cellGap.x = std::max(component.m_cellGap.x, -component.m_cellSize.x + 0.01f);
			component.m_cellGap.y = std::max(component.m_cellGap.y, -component.m_cellSize.y + 0.01f);

			if (dt) dt->MarkSceneDirty();
		}

		const char* currentSwizzle = TilemapUtils::GridCellSwizzleToString(component.m_cellSwizzle);
		if (ImGui::BeginCombo("Cell Swizzle", currentSwizzle))
		{
			for (const GridCellSwizzleEntry& entry : s_cellSwizzleEntries)
			{
				bool selected = component.m_cellSwizzle == entry.swizzle;
				if (ImGui::Selectable(entry.label, selected))
				{
					component.m_cellSwizzle = entry.swizzle;
					if (dt) dt->MarkSceneDirty();
				}

				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}

}
