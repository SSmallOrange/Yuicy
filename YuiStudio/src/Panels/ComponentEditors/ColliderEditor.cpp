#include "pch.h"

#include "ColliderEditor.h"

#include "Yuicy/Project/Project.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Tilemap/TilemapColliderGeometry.h"

#include "imgui/imgui.h"

#include <glm/gtc/type_ptr.hpp>

#include <vector>

namespace Yuicy {

	namespace {

		struct TileColliderTypeEntry
		{
			TileColliderType colliderType;
			const char* label;
		};

		struct TilemapColliderCompositeOperationEntry
		{
			TilemapColliderCompositeOperation operation;
			const char* label;
		};

		constexpr TileColliderTypeEntry tileColliderTypeEntries[] = {
			{ TileColliderType::None,   "None" },
			{ TileColliderType::Grid,   "Grid" },
			{ TileColliderType::Sprite, "Sprite" }
		};

		constexpr TilemapColliderCompositeOperationEntry tilemapColliderCompositeOperationEntries[] = {
			{ TilemapColliderCompositeOperation::None,  "None" },
			{ TilemapColliderCompositeOperation::Merge, "Merge" }
		};

		const GridComponent* ResolveGridForTilemap(Entity tilemapEntity)
		{
			if (tilemapEntity.HasComponent<GridComponent>())
				return &tilemapEntity.GetComponent<GridComponent>();

			Entity parent = tilemapEntity.GetParent();
			if (parent && parent.HasComponent<GridComponent>())
				return &parent.GetComponent<GridComponent>();

			return nullptr;
		}

		std::vector<TilemapColliderShape> BuildTilemapColliderShapes(
			const glm::mat4& tilemapWorldTransform,
			const GridComponent& grid,
			const TilemapComponent& tilemap,
			const TilemapCollider2DComponent& collider)
		{
			switch (collider.compositeOperation)
			{
				case TilemapColliderCompositeOperation::None:
					return TilemapColliderGeometry::BuildGridShapes(tilemapWorldTransform, grid, tilemap, collider);
				case TilemapColliderCompositeOperation::Merge:
					return TilemapColliderGeometry::BuildMergedGridShapes(tilemapWorldTransform, grid, tilemap, collider);
			}

			return TilemapColliderGeometry::BuildMergedGridShapes(tilemapWorldTransform, grid, tilemap, collider);
		}

		int EstimateTilemapShapeCount(Entity tilemapEntity, const TilemapCollider2DComponent& collider)
		{
			if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
				return -1;

			Scene* scene = tilemapEntity.GetScene();
			const GridComponent* grid = ResolveGridForTilemap(tilemapEntity);
			if (!scene || !grid)
				return -1;

			const auto& tilemap = tilemapEntity.GetComponent<TilemapComponent>();
			const glm::mat4 tilemapWorldTransform = scene->GetWorldSpaceTransformMatrix(tilemapEntity);
			std::vector<TilemapColliderShape> shapes = BuildTilemapColliderShapes(tilemapWorldTransform, *grid, tilemap, collider);
			return (int)shapes.size();
		}

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

	void ColliderEditor::DrawTilemapCollider(TilemapCollider2DComponent& component, Entity entity, EditorDirtyTracker* dt)
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

		const char* currentCompositeOperation = TilemapUtils::TilemapColliderCompositeOperationToString(component.compositeOperation);
		if (ImGui::BeginCombo("Composite Operation", currentCompositeOperation))
		{
			for (const TilemapColliderCompositeOperationEntry& entry : tilemapColliderCompositeOperationEntries)
			{
				bool isSelected = component.compositeOperation == entry.operation;
				if (ImGui::Selectable(entry.label, isSelected))
				{
					component.compositeOperation = entry.operation;
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
		const int estimatedShapeCount = EstimateTilemapShapeCount(entity, component);
		if (estimatedShapeCount >= 0)
			ImGui::Text("Estimated Shape Count: %d", estimatedShapeCount);
		else
			ImGui::TextDisabled("Estimated Shape Count: N/A");

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
