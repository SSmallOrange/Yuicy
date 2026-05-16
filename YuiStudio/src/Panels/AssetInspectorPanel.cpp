#include "pch.h"

#include "AssetInspectorPanel.h"

#include "../Editor/EditorContext.h"
#include "../Editor/EditorAssetWorkflow.h"

#include "Yuicy/Asset/AssetImporter.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Asset/AssetTypes.h"
#include "Yuicy/Sprite/SpriteAsset.h"
#include "Yuicy/Tilemap/Tile.h"
#include "Yuicy/Tilemap/TilePalette.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Project/Project.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>
#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	namespace {

		constexpr ImVec4 s_missingColor = ImVec4(1.0f, 0.35f, 0.35f, 1.0f);

		const char* GetAssetDropHint(AssetType assetType)
		{
			switch (assetType)
			{
				case AssetType::Texture:     return "Drop Texture Here";
				case AssetType::Sprite:      return "Drop Sprite Here";
				case AssetType::Tile:        return "Drop Tile Here";
				case AssetType::TilePalette: return "Drop Tile Palette Here";
				default:                     return "Drop Asset Here";
			}
		}

		void ClampVec2(glm::vec2& value, float minValue, float maxValue)
		{
			value.x = std::clamp(value.x, minValue, maxValue);
			value.y = std::clamp(value.y, minValue, maxValue);
		}

		bool DrawAssetReference(const char* label, AssetHandle handle, AssetType expectedType)
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (!assetManager)
				return false;

			ImGui::Text("%s", label);
			ImGui::Indent();

			if (handle == 0)
			{
				ImGui::TextDisabled("None");
				ImGui::Unindent();
				return false;
			}

			if (!assetManager->IsAssetHandleValid(handle))
			{
				ImGui::TextColored(s_missingColor, "Missing asset handle: %llu", (unsigned long long)(uint64_t)handle);
				ImGui::Unindent();
				return false;
			}

			const AssetMetadata& metadata = assetManager->GetMetadata(handle);
			if (!metadata.IsValid())
			{
				ImGui::TextColored(s_missingColor, "Missing metadata: %llu", (unsigned long long)(uint64_t)handle);
				ImGui::Unindent();
				return false;
			}

			if (expectedType != AssetType::None && metadata.type != expectedType)
			{
				ImGui::TextColored(s_missingColor, "Type mismatch: %s", Utils::AssetTypeToString(metadata.type));
				ImGui::TextDisabled("Expected: %s", Utils::AssetTypeToString(expectedType));
				ImGui::Unindent();
				return false;
			}

			ImGui::Text("%s", metadata.filePath.filename().string().c_str());
			ImGui::TextDisabled("Path: %s", metadata.filePath.string().c_str());
			ImGui::TextDisabled("Handle: %llu", (unsigned long long)(uint64_t)handle);

			ImGui::Unindent();
			return true;
		}

		bool DrawAssetDropTarget(const char* id, AssetHandle& handle, AssetType expectedType)
		{
			bool changed = false;
			auto assetManager = Project::GetEditorAssetManager();

			std::string buttonLabel = handle == 0 ? GetAssetDropHint(expectedType) : "Replace Asset";
			ImGui::Button((buttonLabel + "##" + id).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0));

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const auto* pathData = (const std::filesystem::path::value_type*)payload->Data;
					std::filesystem::path filepath(pathData);

					if (assetManager && assetManager->GetAssetTypeFromPath(filepath) == expectedType)
					{
						AssetHandle droppedHandle = assetManager->ImportAsset(filepath);
						if (droppedHandle != 0)
						{
							handle = droppedHandle;
							changed = true;
						}
					}
				}

				ImGui::EndDragDropTarget();
			}

			if (handle != 0)
			{
				if (ImGui::SmallButton((std::string("Clear##") + id).c_str()))
				{
					handle = 0;
					changed = true;
				}
			}

			return changed;
		}

		void DrawTexturePreview(const Ref<Texture2D>& texture, glm::vec2 uvMin = { 0.0f, 0.0f }, glm::vec2 uvMax = { 1.0f, 1.0f })
		{
			if (!texture)
			{
				ImGui::TextColored(s_missingColor, "Preview texture is missing.");
				return;
			}

			const float uvWidth = std::max(std::abs(uvMax.x - uvMin.x), 0.001f);
			const float uvHeight = std::max(std::abs(uvMax.y - uvMin.y), 0.001f);
			const float textureWidth = (float)texture->GetWidth() * uvWidth;
			const float textureHeight = (float)texture->GetHeight() * uvHeight;
			const float aspect = textureHeight > 0.0f ? textureWidth / textureHeight : 1.0f;

			float previewMaxWidth = ImGui::GetContentRegionAvail().x;
			float previewMaxHeight = 192.0f;
			float previewWidth = previewMaxWidth;
			float previewHeight = previewWidth / aspect;
			if (previewHeight > previewMaxHeight)
			{
				previewHeight = previewMaxHeight;
				previewWidth = previewHeight * aspect;
			}

			ImTextureID texId = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
			ImGui::Image(texId, ImVec2{ previewWidth, previewHeight },
				ImVec2{ uvMin.x, uvMax.y }, ImVec2{ uvMax.x, uvMin.y });
		}

		Ref<Texture2D> GetTextureFromSprite(const Ref<SpriteAsset>& sprite)
		{
			if (!sprite || sprite->m_textureHandle == 0)
				return nullptr;

			auto assetManager = Project::GetEditorAssetManager();
			if (!assetManager || !assetManager->IsAssetHandleValid(sprite->m_textureHandle))
				return nullptr;

			if (assetManager->GetAssetType(sprite->m_textureHandle) != AssetType::Texture)
				return nullptr;

			return assetManager->GetAsset<Texture2D>(sprite->m_textureHandle);
		}

		bool SaveAsset(const AssetMetadata& metadata, const Ref<Asset>& asset)
		{
			if (!asset)
				return false;

			AssetImporter::Serialize(metadata, asset);

			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
				assetManager->ReloadData(metadata.handle);

			YUICY_CORE_INFO("[AssetInspector] Saved asset: {}", metadata.filePath.string());
			return true;
		}

	}

	void AssetInspectorPanel::OnImGuiRender()
	{
		ImGui::Begin("Asset Inspector");

		if (!m_context || !m_context->selection.HasAssetSelection())
		{
			DrawNoSelection();
			ImGui::End();
			return;
		}

		AssetHandle selectedHandle = m_context->selection.selectedAsset;
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || !assetManager->IsAssetHandleValid(selectedHandle))
		{
			ImGui::TextDisabled("Invalid asset handle.");
			ImGui::End();
			return;
		}

		const AssetMetadata& metadata = assetManager->GetMetadata(selectedHandle);
		if (!metadata.IsValid())
		{
			ImGui::TextDisabled("Asset metadata not found.");
			ImGui::End();
			return;
		}

		DrawAssetHeader(metadata);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		switch (metadata.type)
		{
		case AssetType::Texture:  DrawTextureInspector(metadata); break;
		case AssetType::Scene:    DrawSceneInspector(metadata);   break;
		case AssetType::LuaScript: DrawScriptInspector(metadata); break;
		case AssetType::Shader:   DrawShaderInspector(metadata);  break;
		case AssetType::Font:     DrawFontInspector(metadata);    break;
		case AssetType::Sprite:   DrawSpriteInspector(metadata);  break;
		case AssetType::Tile:     DrawTileInspector(metadata);    break;
		case AssetType::TilePalette: DrawTilePaletteInspector(metadata); break;
		default:
			ImGui::TextDisabled("No inspector available for this asset type.");
			break;
		}

		ImGui::End();
	}

	void AssetInspectorPanel::DrawNoSelection()
	{
		float availWidth = ImGui::GetContentRegionAvail().x;
		float availHeight = ImGui::GetContentRegionAvail().y;

		const char* hint = "Select an asset in the Content Browser\nto inspect its properties.";
		ImVec2 textSize = ImGui::CalcTextSize(hint);

		ImGui::SetCursorPosX((availWidth - textSize.x) * 0.5f);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + availHeight * 0.3f);
		ImGui::TextDisabled("%s", hint);
	}

	void AssetInspectorPanel::DrawAssetHeader(const AssetMetadata& metadata)
	{
		// 文件名
		std::string filename = metadata.filePath.filename().string();
		ImGui::Text("%s", filename.c_str());

		// 资源类型标签
		ImGui::SameLine();
		const char* typeStr = Utils::AssetTypeToString(metadata.type);
		ImGui::TextDisabled("[%s]", typeStr);

		// 相对路径
		ImGui::TextDisabled("Path: %s", metadata.filePath.string().c_str());

		// Handle ID
		ImGui::TextDisabled("Handle: %llu", (unsigned long long)(uint64_t)metadata.handle);

		// 加载状态
		ImGui::TextDisabled("Loaded: %s", metadata.isDataLoaded ? "Yes" : "No");

		// 操作按钮
		ImGui::Spacing();

		if (ImGui::Button("Reload"))
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
				assetManager->ReloadData(metadata.handle);
		}

		ImGui::SameLine();

		if (ImGui::Button("Open"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
				m_assetWorkflow->OpenAsset(fullPath);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Show in Explorer"))
		{
			if (m_assetWorkflow)
			{
				std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
				m_assetWorkflow->RevealInExplorer(fullPath.parent_path());
			}
		}
	}

	void AssetInspectorPanel::DrawTextureInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Texture Properties");
		ImGui::Spacing();

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		Ref<Texture2D> texture = assetManager->GetAsset<Texture2D>(metadata.handle);
		if (!texture)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Failed to load texture data.");
			return;
		}

		uint32_t width = texture->GetWidth();
		uint32_t height = texture->GetHeight();

		ImGui::Text("Width:  %u px", width);
		ImGui::Text("Height: %u px", height);

		// 文件大小
		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024 * 1024)
				ImGui::Text("File Size: %.2f MB", fileSize / (1024.0f * 1024.0f));
			else if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 纹理预览
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Preview");
		ImGui::Spacing();

		float previewMaxWidth = ImGui::GetContentRegionAvail().x;
		float previewMaxHeight = 256.0f;
		float aspect = (height > 0) ? (float)width / (float)height : 1.0f;

		float previewWidth = previewMaxWidth;
		float previewHeight = previewWidth / aspect;
		if (previewHeight > previewMaxHeight)
		{
			previewHeight = previewMaxHeight;
			previewWidth = previewHeight * aspect;
		}

		ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
		ImGui::Image(texID, ImVec2{ previewWidth, previewHeight }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}

	void AssetInspectorPanel::DrawSceneInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Scene Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 最后修改时间
		auto lastWrite = std::filesystem::last_write_time(fullPath, ec);
		if (!ec)
		{
			auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(lastWrite);
			auto time = std::chrono::system_clock::to_time_t(sctp);
			std::tm tm{};
			localtime_s(&tm, &time);
			char timeBuf[64];
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
			ImGui::Text("Last Modified: %s", timeBuf);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open Scene"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenAsset(fullPath);
		}
	}

	void AssetInspectorPanel::DrawScriptInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Lua Script Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		// 最后修改时间
		auto lastWrite = std::filesystem::last_write_time(fullPath, ec);
		if (!ec)
		{
			auto sctp = std::chrono::clock_cast<std::chrono::system_clock>(lastWrite);
			auto time = std::chrono::system_clock::to_time_t(sctp);
			std::tm tm{};
			localtime_s(&tm, &time);
			char timeBuf[64];
			std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);
			ImGui::Text("Last Modified: %s", timeBuf);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open in Editor"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenFileExternal(fullPath);
		}

		ImGui::SameLine();

		if (ImGui::Button("Reload Script"))
		{
			auto assetManager = Project::GetEditorAssetManager();
			if (assetManager)
				assetManager->ReloadData(metadata.handle);
		}
	}

	void AssetInspectorPanel::DrawShaderInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Shader Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}

		ImGui::Spacing();

		if (ImGui::Button("Open in Editor"))
		{
			if (m_assetWorkflow)
				m_assetWorkflow->OpenFileExternal(fullPath);
		}
	}

	void AssetInspectorPanel::DrawFontInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Font Properties");
		ImGui::Spacing();

		std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);

		// 文件大小
		std::error_code ec;
		auto fileSize = std::filesystem::file_size(fullPath, ec);
		if (!ec)
		{
			if (fileSize >= 1024)
				ImGui::Text("File Size: %.1f KB", fileSize / 1024.0f);
			else
				ImGui::Text("File Size: %llu bytes", (unsigned long long)fileSize);
		}
	}

	void AssetInspectorPanel::DrawSpriteInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Sprite Properties");
		ImGui::Spacing();

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		Ref<SpriteAsset> sprite = assetManager->GetAsset<SpriteAsset>(metadata.handle);
		if (!sprite)
		{
			ImGui::TextColored(s_missingColor, "Failed to load sprite data.");
			return;
		}

		DrawAssetReference("Texture", sprite->m_textureHandle, AssetType::Texture);
		DrawAssetDropTarget("SpriteTexture", sprite->m_textureHandle, AssetType::Texture);

		ImGui::Spacing();
		if (ImGui::DragFloat2("UV Min", glm::value_ptr(sprite->m_uvMin), 0.01f, 0.0f, 1.0f, "%.3f"))
			ClampVec2(sprite->m_uvMin, 0.0f, 1.0f);

		if (ImGui::DragFloat2("UV Max", glm::value_ptr(sprite->m_uvMax), 0.01f, 0.0f, 1.0f, "%.3f"))
			ClampVec2(sprite->m_uvMax, 0.0f, 1.0f);

		if (ImGui::DragFloat2("Pivot", glm::value_ptr(sprite->m_pivot), 0.01f, 0.0f, 1.0f, "%.3f"))
			ClampVec2(sprite->m_pivot, 0.0f, 1.0f);

		ImGui::DragFloat4("Border", glm::value_ptr(sprite->m_border), 0.01f, 0.0f, 10000.0f, "%.2f");

		if (ImGui::DragFloat("Pixels Per Unit", &sprite->m_pixelsPerUnit, 1.0f, 1.0f, 10000.0f, "%.1f"))
			sprite->m_pixelsPerUnit = std::max(sprite->m_pixelsPerUnit, 1.0f);

		ImGui::Spacing();
		if (ImGui::Button("Save Sprite"))
			SaveAsset(metadata, std::static_pointer_cast<Asset>(sprite));

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Preview");
		ImGui::Spacing();

		DrawTexturePreview(GetTextureFromSprite(sprite), sprite->m_uvMin, sprite->m_uvMax);
	}

	void AssetInspectorPanel::DrawTileInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Tile Properties");
		ImGui::Spacing();

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		Ref<TileAsset> tile = assetManager->GetAsset<TileAsset>(metadata.handle);
		if (!tile)
		{
			ImGui::TextColored(s_missingColor, "Failed to load tile data.");
			return;
		}

		DrawAssetReference("Sprite", tile->m_spriteHandle, AssetType::Sprite);
		DrawAssetDropTarget("TileSprite", tile->m_spriteHandle, AssetType::Sprite);

		ImGui::Spacing();
		ImGui::ColorEdit4("Color", glm::value_ptr(tile->m_color));

		const char* colliderLabels[] = { "None", "Sprite", "Grid" };
		int colliderIndex = (int)tile->m_colliderType;
		if (colliderIndex < 0 || colliderIndex >= 3)
			colliderIndex = 0;

		if (ImGui::BeginCombo("Collider Type", colliderLabels[colliderIndex]))
		{
			for (int i = 0; i < 3; i++)
			{
				bool selected = colliderIndex == i;
				if (ImGui::Selectable(colliderLabels[i], selected))
				{
					colliderIndex = i;
					tile->m_colliderType = (TileColliderType)i;
				}

				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		int flags = (int)tile->m_flags;
		if (ImGui::InputInt("Flags", &flags))
			tile->m_flags = (uint32_t)std::max(flags, 0);

		ImGui::Spacing();
		if (ImGui::Button("Save Tile"))
			SaveAsset(metadata, std::static_pointer_cast<Asset>(tile));

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Text("Preview");
		ImGui::Spacing();

		Ref<SpriteAsset> sprite = nullptr;
		if (tile->m_spriteHandle != 0 && assetManager->IsAssetHandleValid(tile->m_spriteHandle)
			&& assetManager->GetAssetType(tile->m_spriteHandle) == AssetType::Sprite)
		{
			sprite = assetManager->GetAsset<SpriteAsset>(tile->m_spriteHandle);
		}

		if (sprite)
			DrawTexturePreview(GetTextureFromSprite(sprite), sprite->m_uvMin, sprite->m_uvMax);
		else
			ImGui::TextColored(s_missingColor, "Tile sprite is missing.");
	}

	void AssetInspectorPanel::DrawTilePaletteInspector(const AssetMetadata& metadata)
	{
		ImGui::Text("Tile Palette Properties");
		ImGui::Spacing();

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager)
			return;

		Ref<TilePaletteAsset> palette = assetManager->GetAsset<TilePaletteAsset>(metadata.handle);
		if (!palette)
		{
			ImGui::TextColored(s_missingColor, "Failed to load tile palette data.");
			return;
		}

		std::array<char, 256> nameBuffer{};
		std::memcpy(nameBuffer.data(), palette->m_name.c_str(), std::min(palette->m_name.size(), nameBuffer.size() - 1));
		if (ImGui::InputText("Name", nameBuffer.data(), nameBuffer.size()))
			palette->m_name = std::string(nameBuffer.data());

		const char* layoutLabels[] = { "Rectangular", "Isometric", "Hexagonal" };
		int layoutIndex = (int)palette->m_layout;
		if (layoutIndex < 0 || layoutIndex >= 3)
			layoutIndex = 0;

		if (ImGui::BeginCombo("Layout", layoutLabels[layoutIndex]))
		{
			for (int i = 0; i < 3; i++)
			{
				bool selected = layoutIndex == i;
				if (ImGui::Selectable(layoutLabels[i], selected))
				{
					layoutIndex = i;
					palette->m_layout = (GridLayout)i;
				}

				if (selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

		if (ImGui::DragFloat2("Cell Size", glm::value_ptr(palette->m_cellSize), 0.05f, 0.01f, 100.0f, "%.2f"))
		{
			palette->m_cellSize.x = std::max(palette->m_cellSize.x, 0.01f);
			palette->m_cellSize.y = std::max(palette->m_cellSize.y, 0.01f);
		}

		ImGui::DragFloat2("Cell Gap", glm::value_ptr(palette->m_cellGap), 0.01f, -100.0f, 100.0f, "%.2f");
		ImGui::Text("Cell Count: %zu", palette->m_cells.size());

		ImGui::Spacing();
		if (ImGui::Button("Save Tile Palette"))
			SaveAsset(metadata, std::static_pointer_cast<Asset>(palette));
	}

}
