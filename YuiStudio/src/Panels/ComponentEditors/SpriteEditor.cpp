#include "pch.h"

#include "SpriteEditor.h"
#include "../../Editor/EditorContext.h"
#include "../../Editor/EditorDirtyTracker.h"
#include "../../Utils/ContentBrowserDragDrop.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Renderer/SortingLayerConfig.h"
#include "Yuicy/Scene/Components.h"

#include <filesystem>
#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	void SpriteEditor::Draw(SpriteRendererComponent& component, EditorDirtyTracker* dt)
	{
		const EditorProjectSession* project = m_editorContext ? m_editorContext->project.get() : nullptr;

		if (ImGui::ColorEdit4("Color", glm::value_ptr(component.Color)))
			if (dt) dt->MarkSceneDirty();

		// 纹理预览与拖拽
		std::string textureName = "None";
		std::string texturePath;
		EditorAssetManager* assetManager = project ? project->GetAssetManager().get() : nullptr;
		bool hasTexture = component.TextureHandle != 0
			&& assetManager && assetManager->IsAssetHandleValid(component.TextureHandle);

		Ref<Texture2D> texture = nullptr;
		if (hasTexture)
		{
			const auto& metadata = assetManager->GetMetadata(component.TextureHandle);
			if (metadata.IsValid())
			{
				textureName = metadata.filePath.filename().string();
				texturePath = metadata.filePath.string();
			}
			texture = assetManager->GetAssetAs<Texture2D>(component.TextureHandle);
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
			if (std::optional<std::filesystem::path> droppedPath = ContentBrowserDragDrop::AcceptPayload())
			{
				std::filesystem::path filepath = *droppedPath;

				if (assetManager && assetManager->GetAssetTypeFromPath(filepath) == AssetType::Texture)
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
			static const SortingLayerConfig kDefaultSortingLayers;
			const SortingLayerConfig& sortingLayers = project ? project->GetProject().GetSettings().Renderer2D.SortingLayers : kDefaultSortingLayers;
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
