#include "pch.h"

#include "ColliderEditor.h"

#include "Yuicy/Project/Project.h"

#include "imgui/imgui.h"

#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	namespace {

		struct TileColliderTypeEntry
		{
			TileColliderType colliderType;
			const char* label;
		};

		constexpr TileColliderTypeEntry tileColliderTypeEntries[] = {
			{ TileColliderType::None,   "None" },
			{ TileColliderType::Grid,   "Grid" },
			{ TileColliderType::Sprite, "Sprite" }
		};

	}

	void ColliderEditor::DrawBoxCollider(BoxCollider2DComponent& component, EditorDirtyTracker* dt)
	{
		if (ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat2("Size", glm::value_ptr(component.Size), 0.01f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 10.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::Checkbox("Is Trigger", &component.IsTrigger))
			if (dt) dt->MarkSceneDirty();

		ImGui::Separator();
		DrawCollisionFilter(component.CategoryBits, component.MaskBits, dt);
	}

	void ColliderEditor::DrawCircleCollider(CircleCollider2DComponent& component, EditorDirtyTracker* dt)
	{
		if (ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Radius", &component.Radius, 0.01f, 0.0f, 10.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 10.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::Checkbox("Is Trigger", &component.IsTrigger))
			if (dt) dt->MarkSceneDirty();

		ImGui::Separator();
		DrawCollisionFilter(component.CategoryBits, component.MaskBits, dt);
	}

	void ColliderEditor::DrawTilemapCollider(TilemapCollider2DComponent& component, EditorDirtyTracker* dt)
	{
		const char* currentColliderType = TilemapUtils::TileColliderTypeToString(component.defaultColliderType);
		if (ImGui::BeginCombo("Default Collider Type", currentColliderType))
		{
			for (const TileColliderTypeEntry& entry : tileColliderTypeEntries)
			{
				bool isSelected = component.defaultColliderType == entry.colliderType;
				if (ImGui::Selectable(entry.label, isSelected))
				{
					component.defaultColliderType = entry.colliderType;
					if (dt) dt->MarkSceneDirty();
				}

				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		if (ImGui::Checkbox("Is Trigger", &component.isTrigger))
			if (dt) dt->MarkSceneDirty();

		if (ImGui::DragFloat("Density", &component.density, 0.01f, 0.0f, 10.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Friction", &component.friction, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution", &component.restitution, 0.01f, 0.0f, 1.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::DragFloat("Restitution Threshold", &component.restitutionThreshold, 0.01f, 0.0f))
			if (dt) dt->MarkSceneDirty();

		ImGui::Text("Runtime Shape Count: %d", (int)component.runtimeFixtures.size());

		ImGui::Separator();
		DrawCollisionFilter(component.categoryBits, component.maskBits, dt);
	}

	// 从 bitmask 中找到最低位的索引（用于单选下拉框显示）
	static int GetLowestBitIndex(uint16_t bits)
	{
		for (int i = 0; i < CollisionLayerConfig::MaxLayers; i++)
		{
			if (bits & (1 << i))
				return i;
		}
		return 0;
	}

	void ColliderEditor::DrawCollisionFilter(uint16_t& categoryBits, uint16_t& maskBits, EditorDirtyTracker* dt)
	{
		auto project = Project::GetActive();
		if (!project) return;

		const auto& layerConfig = project->GetConfig().CollisionLayers;

		// Category：单选下拉框
		int currentLayer = GetLowestBitIndex(categoryBits);
		if (ImGui::BeginCombo("Layer", layerConfig.LayerNames[currentLayer].c_str()))
		{
			for (int i = 0; i < CollisionLayerConfig::MaxLayers; i++)
			{
				bool isSelected = (currentLayer == i);
				if (ImGui::Selectable(layerConfig.LayerNames[i].c_str(), isSelected))
				{
					categoryBits = 1 << i;
					if (dt) dt->MarkSceneDirty();
				}
				if (isSelected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		// Mask：折叠式碰撞对象选择
		if (ImGui::TreeNode("Collides With"))
		{
			// All / None 快捷按钮
			if (ImGui::SmallButton("All"))
			{
				maskBits = 0xFFFF;
				if (dt) dt->MarkSceneDirty();
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("None"))
			{
				maskBits = 0;
				if (dt) dt->MarkSceneDirty();
			}

			for (int i = 0; i < CollisionLayerConfig::MaxLayers; i++)
			{
				uint16_t bit = 1 << i;
				bool enabled = (maskBits & bit) != 0;

				ImGui::PushID(i);
				if (ImGui::Checkbox(layerConfig.LayerNames[i].c_str(), &enabled))
				{
					if (enabled)
						maskBits |= bit;
					else
						maskBits &= ~bit;
					if (dt) dt->MarkSceneDirty();
				}
				ImGui::PopID();
			}

			ImGui::TreePop();
		}
	}

}
