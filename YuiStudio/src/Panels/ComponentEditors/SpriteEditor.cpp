#include "pch.h"

#include "SpriteEditor.h"
#include "../../Editor/EditorDirtyTracker.h"

#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Scene/Components.h"

#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	void SpriteEditor::Draw(SpriteRendererComponent& component, EditorDirtyTracker* dt)
	{
		if (ImGui::ColorEdit4("Color", glm::value_ptr(component.Color)))
			if (dt) dt->MarkSceneDirty();

		// 纹理预览与拖拽
		std::string textureName = "None";
		std::string texturePath;
		bool hasTexture = component.TextureHandle != 0
			&& AssetManager::IsAssetHandleValid(component.TextureHandle);

		Ref<Texture2D> texture = nullptr;
		if (hasTexture)
		{
			const auto& metadata = Project::GetEditorAssetManager()->GetMetadata(component.TextureHandle);
			if (metadata.IsValid())
			{
				textureName = metadata.filePath.filename().string();
				texturePath = metadata.filePath.string();
			}
			texture = AssetManager::GetAsset<Texture2D>(component.TextureHandle);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
		ImVec2 textureCursorPos = ImGui::GetCursorPos();
		const float thumbnailSize = 64.0f;

		if (hasTexture && texture)
		{
			ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
			ImGui::Image(texID, ImVec2{ thumbnailSize, thumbnailSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
		}
		else
		{
			ImGui::Button("Drop\nTexture", ImVec2{ thumbnailSize, thumbnailSize });
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const wchar_t* droppedPath = (const wchar_t*)payload->Data;
				std::filesystem::path filepath = droppedPath;

				auto assetManager = Project::GetEditorAssetManager();
				AssetType type = assetManager->GetAssetTypeFromPath(filepath);
				if (type == AssetType::Texture)
				{
					AssetHandle handle = assetManager->ImportAsset(filepath);
					if (handle != 0)
					{
						component.TextureHandle = handle;
						if (dt) dt->MarkSceneDirty();
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		ImGui::PopStyleVar();

		// 纹理 Tooltip
		if (ImGui::IsItemHovered() && hasTexture && texture)
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(texturePath.c_str());
			ImGui::PopTextWrapPos();
			ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
			ImGui::Image(texID, ImVec2(256, 256), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
			ImGui::EndTooltip();
		}

		// 纹理名称与清除按钮
		ImVec2 nextRowCursorPos = ImGui::GetCursorPos();
		ImGui::SameLine();
		ImVec2 rightOfImagePos = ImGui::GetCursorPos();

		ImGui::SetCursorPos(textureCursorPos);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		if (hasTexture && ImGui::Button("X##ClearTexture", ImVec2(18, 18)))
		{
			component.TextureHandle = 0;
			if (dt) dt->MarkSceneDirty();
		}
		ImGui::PopStyleVar();

		ImGui::SetCursorPos(rightOfImagePos);
		ImGui::Text("%s", textureName.c_str());

		ImGui::SetCursorPos(nextRowCursorPos);

		if (ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f))
			if (dt) dt->MarkSceneDirty();
		if (ImGui::Checkbox("Flip X", &component.FlipX))
			if (dt) dt->MarkSceneDirty();
		ImGui::SameLine();
		if (ImGui::Checkbox("Flip Y", &component.FlipY))
			if (dt) dt->MarkSceneDirty();

		// Sorting Layer
		{
			auto project = Project::GetActive();
			const auto& sortingLayers = project->GetConfig().SortingLayers;
			if (ImGui::BeginCombo("Sorting Layer", component.SortingLayer.c_str()))
			{
				for (const auto& layer : sortingLayers.Layers)
				{
					bool isSelected = (component.SortingLayer == layer.Name);
					if (ImGui::Selectable(layer.Name.c_str(), isSelected))
					{
						component.SortingLayer = layer.Name;
						if (dt) dt->MarkSceneDirty();
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		if (ImGui::DragInt("Order in Layer", &component.SortingOrder))
			if (dt) dt->MarkSceneDirty();
	}

}
