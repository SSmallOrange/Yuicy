#include "pch.h"

#include "TilePalettePanel.h"

#include "../Editor/EditorAssetWorkflow.h"
#include "../Editor/EditorContext.h"
#include "../Editor/EditorDirtyTracker.h"

#include "Yuicy/Asset/AssetImporter.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Renderer/Texture.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Sprite/SpriteAsset.h"
#include "Yuicy/Tilemap/Tile.h"
#include "Yuicy/Tilemap/TilePalette.h"

#include "imgui/imgui.h"

#include <algorithm>
#include <cmath>
#include <cfloat>
#include <filesystem>
#include <string>
#include <vector>

namespace Yuicy {

	namespace {

		constexpr float s_paletteCellSize = 34.0f;
		constexpr float s_previewMaxSize = 128.0f;

		struct AssetEntry
		{
			AssetHandle m_handle = 0;
			std::string m_label;
		};

		struct TilemapTargetEntry
		{
			UUID m_uuid = 0;
			std::string m_label;
		};

		struct TilemapToolEntry
		{
			TilemapTool m_tool = TilemapTool::Select;
			const char* m_label = "";
		};

		constexpr TilemapToolEntry s_toolEntries[] = {
			{ TilemapTool::Select,    "Select" },
			{ TilemapTool::Move,      "Move" },
			{ TilemapTool::Paint,     "Paint" },
			{ TilemapTool::Erase,     "Erase" },
			{ TilemapTool::BoxFill,   "Box Fill" },
			{ TilemapTool::FloodFill, "Flood Fill" },
			{ TilemapTool::Picker,    "Picker" }
		};

		std::string FormatHandle(AssetHandle handle)
		{
			return std::to_string((uint64_t)handle);
		}

		std::string FormatUUID(UUID uuid)
		{
			return std::to_string((uint64_t)uuid);
		}

		std::string GetAssetLabel(const Ref<EditorAssetManager>& assetManager, AssetHandle handle, AssetType expectedType)
		{
			if (!assetManager || handle == 0)
				return "None";

			if (!assetManager->IsAssetHandleValid(handle))
				return "Missing: " + FormatHandle(handle);

			const AssetMetadata& metadata = assetManager->GetMetadata(handle);
			if (!metadata.IsValid())
				return "Missing Metadata: " + FormatHandle(handle);

			if (metadata.type != expectedType)
				return std::string("Type Mismatch: ") + Utils::AssetTypeToString(metadata.type);

			std::string filename = metadata.filePath.filename().string();
			return filename.empty() ? FormatHandle(handle) : filename;
		}

		std::vector<AssetEntry> CollectAssetsWithType(const Ref<EditorAssetManager>& assetManager, AssetType assetType)
		{
			std::vector<AssetEntry> entries;
			if (!assetManager)
				return entries;

			auto handles = assetManager->GetAllAssetsWithType(assetType);
			entries.reserve(handles.size());

			for (AssetHandle handle : handles)
			{
				const AssetMetadata& metadata = assetManager->GetMetadata(handle);
				if (!metadata.IsValid())
					continue;

				std::string label = metadata.filePath.filename().string();
				if (label.empty())
					label = FormatHandle(handle);

				entries.push_back({ handle, label });
			}

			std::sort(entries.begin(), entries.end(), [](const AssetEntry& lhs, const AssetEntry& rhs)
			{
				return lhs.m_label < rhs.m_label;
			});

			return entries;
		}

		std::string GetEntityLabel(Entity entity)
		{
			if (!entity)
				return "None";

			std::string label = "Tilemap";
			if (entity.HasComponent<TagComponent>())
			{
				const auto& tag = entity.GetComponent<TagComponent>().Tag;
				if (!tag.empty())
					label = tag;
			}

			return label + " (" + FormatUUID(entity.GetUUID()) + ")";
		}

		void DrawTexturePreview(const Ref<Texture2D>& texture, glm::vec2 uvMin, glm::vec2 uvMax, glm::vec4 tint)
		{
			if (!texture)
			{
				ImGui::Button("##MissingTilePreview", ImVec2{ s_previewMaxSize, s_previewMaxSize });
				return;
			}

			const float uvWidth = std::max(std::abs(uvMax.x - uvMin.x), 0.001f);
			const float uvHeight = std::max(std::abs(uvMax.y - uvMin.y), 0.001f);
			const float textureWidth = (float)texture->GetWidth() * uvWidth;
			const float textureHeight = (float)texture->GetHeight() * uvHeight;
			const float aspect = textureHeight > 0.0f ? textureWidth / textureHeight : 1.0f;

			float previewWidth = s_previewMaxSize;
			float previewHeight = previewWidth / aspect;
			if (previewHeight > s_previewMaxSize)
			{
				previewHeight = s_previewMaxSize;
				previewWidth = previewHeight * aspect;
			}

			ImTextureID textureId = reinterpret_cast<ImTextureID>((uintptr_t)texture->GetRendererID());
			ImGui::Image(textureId, ImVec2{ previewWidth, previewHeight },
				ImVec2{ uvMin.x, uvMax.y }, ImVec2{ uvMax.x, uvMin.y },
				ImVec4{ tint.r, tint.g, tint.b, tint.a });
		}

	}

	bool TilePalettePanel::IsProjectReady() const
	{
		return Project::GetActive() && Project::GetEditorAssetManager();
	}

	void TilePalettePanel::OnImGuiRender(bool* open)
	{
		if (!open || !(*open))
			return;

		ImGui::Begin("Tile Palette", open);

		if (!m_context)
		{
			ImGui::TextDisabled("Editor context is unavailable.");
			ImGui::End();
			return;
		}

		if (!IsProjectReady())
		{
			DrawProjectDisabledState();
			ImGui::End();
			return;
		}

		DrawPaletteSelector();
		ImGui::Separator();

		DrawActiveTargetSelector();
		ImGui::Separator();

		DrawToolButtons();
		ImGui::Separator();

		DrawPaletteGrid();
		ImGui::Separator();

		DrawActiveTilePreview();

		ImGui::End();
	}

	void TilePalettePanel::DrawProjectDisabledState()
	{
		ImGui::BeginDisabled();
		ImGui::TextDisabled("No active project.");
		ImGui::Button("Create New Palette", ImVec2{ -FLT_MIN, 0.0f });
		ImGui::TextDisabled("Palette: None");
		ImGui::TextDisabled("Active Target: None");
		ImGui::EndDisabled();
	}

	void TilePalettePanel::DrawPaletteSelector()
	{
		auto assetManager = Project::GetEditorAssetManager();
		auto& tilemapState = m_context->tilemap;

		if (tilemapState.m_activePalette != 0
			&& (!assetManager->IsAssetHandleValid(tilemapState.m_activePalette)
				|| assetManager->GetAssetType(tilemapState.m_activePalette) != AssetType::TilePalette))
		{
			tilemapState.m_activePalette = 0;
			tilemapState.m_activeTile = 0;
		}

		std::string currentLabel = GetAssetLabel(assetManager, tilemapState.m_activePalette, AssetType::TilePalette);
		if (ImGui::BeginCombo("Palette", currentLabel.c_str()))
		{
			bool noneSelected = tilemapState.m_activePalette == 0;
			if (ImGui::Selectable("None", noneSelected))
			{
				tilemapState.m_activePalette = 0;
				tilemapState.m_activeTile = 0;
			}
			if (noneSelected)
				ImGui::SetItemDefaultFocus();

			std::vector<AssetEntry> palettes = CollectAssetsWithType(assetManager, AssetType::TilePalette);
			for (const AssetEntry& entry : palettes)
			{
				bool selected = tilemapState.m_activePalette == entry.m_handle;
				std::string itemLabel = entry.m_label + "##" + FormatHandle(entry.m_handle);
				if (ImGui::Selectable(itemLabel.c_str(), selected))
				{
					tilemapState.m_activePalette = entry.m_handle;
					tilemapState.m_activeTile = 0;
				}
				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}

		if (ImGui::Button("Create New Palette"))
			CreateNewPalette();

		ImGui::SameLine();
		bool canSavePalette = tilemapState.m_activePalette != 0;
		if (!canSavePalette)
			ImGui::BeginDisabled();

		if (ImGui::Button("Save Palette"))
			SaveActivePalette();

		if (!canSavePalette)
			ImGui::EndDisabled();

		if (IsActivePaletteDirty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("*");
		}
	}

	void TilePalettePanel::DrawActiveTargetSelector()
	{
		m_context->ValidateTilemapState();

		auto& tilemapState = m_context->tilemap;
		std::string currentLabel = "None";
		if (m_context->activeScene && tilemapState.m_activeTilemapEntity != 0)
		{
			Entity activeTilemap = m_context->activeScene->FindEntityByUUID(tilemapState.m_activeTilemapEntity);
			currentLabel = GetEntityLabel(activeTilemap);
		}

		if (ImGui::BeginCombo("Active Target", currentLabel.c_str()))
		{
			bool noneSelected = tilemapState.m_activeTilemapEntity == 0;
			if (ImGui::Selectable("None", noneSelected))
				tilemapState.m_activeTilemapEntity = 0;
			if (noneSelected)
				ImGui::SetItemDefaultFocus();

			std::vector<TilemapTargetEntry> targets;
			if (m_context->activeScene)
			{
				auto view = m_context->activeScene->GetAllEntitiesWith<TilemapComponent>();
				for (auto entityHandle : view)
				{
					Entity entity(entityHandle, m_context->activeScene.get());
					targets.push_back({ entity.GetUUID(), GetEntityLabel(entity) });
				}
			}

			std::sort(targets.begin(), targets.end(), [](const TilemapTargetEntry& lhs, const TilemapTargetEntry& rhs)
			{
				return lhs.m_label < rhs.m_label;
			});

			for (const TilemapTargetEntry& target : targets)
			{
				bool selected = tilemapState.m_activeTilemapEntity == target.m_uuid;
				std::string itemLabel = target.m_label + "##" + FormatUUID(target.m_uuid);
				if (ImGui::Selectable(itemLabel.c_str(), selected))
					tilemapState.m_activeTilemapEntity = target.m_uuid;
				if (selected)
					ImGui::SetItemDefaultFocus();
			}

			ImGui::EndCombo();
		}
	}

	void TilePalettePanel::DrawToolButtons()
	{
		auto& tilemapState = m_context->tilemap;

		ImGui::TextUnformatted("Tools");

		if (ImGui::BeginTable("##TilemapTools", 4, ImGuiTableFlags_SizingStretchSame))
		{
			for (const TilemapToolEntry& entry : s_toolEntries)
			{
				ImGui::TableNextColumn();

				bool selected = tilemapState.m_activeTool == entry.m_tool;
				if (selected)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				}

				if (ImGui::Button(entry.m_label, ImVec2{ -FLT_MIN, 0.0f }))
					tilemapState.m_activeTool = entry.m_tool;

				if (selected)
					ImGui::PopStyleColor(2);
			}

			ImGui::EndTable();
		}
	}

	void TilePalettePanel::DrawPaletteGrid()
	{
		auto assetManager = Project::GetEditorAssetManager();
		auto& tilemapState = m_context->tilemap;

		ImGui::TextUnformatted("Palette Grid");

		Ref<TilePaletteAsset> palette;
		if (tilemapState.m_activePalette != 0 && assetManager
			&& assetManager->IsAssetHandleValid(tilemapState.m_activePalette)
			&& assetManager->GetAssetType(tilemapState.m_activePalette) == AssetType::TilePalette)
		{
			palette = assetManager->GetAsset<TilePaletteAsset>(tilemapState.m_activePalette);
		}

		int minX = 0;
		int maxX = 7;
		int minY = 0;
		int maxY = 3;

		if (palette)
		{
			for (const auto& [position, tileHandle] : palette->m_cells)
			{
				minX = std::min(minX, position.m_x);
				maxX = std::max(maxX, position.m_x);
				minY = std::min(minY, position.m_y);
				maxY = std::max(maxY, position.m_y);
			}

			ImGui::TextDisabled("Cells: %zu", palette->m_cells.size());
		}
		else
		{
			ImGui::TextDisabled("Cells: 0");
		}

		ImGui::BeginChild("##TilePaletteGrid", ImVec2{ 0.0f, 220.0f }, true);

		for (int y = maxY; y >= minY; y--)
		{
			for (int x = minX; x <= maxX; x++)
			{
				GridPosition position;
				position.m_x = x;
				position.m_y = y;
				position.m_z = 0;

				DrawPaletteCell(palette, position);

				if (x < maxX)
					ImGui::SameLine();
			}
		}

		ImGui::EndChild();
	}

	void TilePalettePanel::DrawPaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position)
	{
		auto assetManager = Project::GetEditorAssetManager();
		AssetHandle tileHandle = palette ? palette->GetTile(position) : AssetHandle(0);
		bool hasTile = tileHandle != 0;
		bool selected = hasTile && m_context && m_context->tilemap.m_activeTile == tileHandle;

		ImGui::PushID(position.m_x);
		ImGui::PushID(position.m_y);
		ImGui::PushID(position.m_z);

		if (!palette)
			ImGui::BeginDisabled();

		if (selected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
		}
		else if (hasTile)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.22f, 0.42f, 0.28f, 1.0f });
		}

		ImGui::Button(hasTile ? "Tile" : "", ImVec2{ s_paletteCellSize, s_paletteCellSize });
		bool cellHovered = ImGui::IsItemHovered();

		if (palette && ImGui::IsItemClicked(ImGuiMouseButton_Left))
			m_context->tilemap.m_activeTile = hasTile ? tileHandle : AssetHandle(0);

		if (palette && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const auto* pathData = (const std::filesystem::path::value_type*)payload->Data;
				std::filesystem::path filepath(pathData);
				AssetHandle droppedTile = ResolveDroppedTile(filepath);
				if (droppedTile != 0)
					SetPaletteCell(palette, position, droppedTile);
			}

			ImGui::EndDragDropTarget();
		}

		if (palette && hasTile && ImGui::BeginPopupContextItem("##PaletteCellContext"))
		{
			if (ImGui::MenuItem("Delete Cell"))
				ErasePaletteCell(palette, position);

			ImGui::EndPopup();
		}

		if (hasTile && cellHovered)
		{
			ImGui::BeginTooltip();
			ImGui::Text("Tile: %s", GetAssetLabel(assetManager, tileHandle, AssetType::Tile).c_str());
			ImGui::EndTooltip();
		}

		if (selected)
			ImGui::PopStyleColor(2);
		else if (hasTile)
			ImGui::PopStyleColor();

		if (!palette)
			ImGui::EndDisabled();

		ImGui::PopID();
		ImGui::PopID();
		ImGui::PopID();
	}

	void TilePalettePanel::SetPaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position, AssetHandle tileHandle)
	{
		if (!palette || tileHandle == 0 || !m_context)
			return;

		palette->SetTile(position, tileHandle);
		m_context->tilemap.m_activeTile = tileHandle;
		MarkPaletteDirty(m_context->tilemap.m_activePalette);
	}

	void TilePalettePanel::ErasePaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position)
	{
		if (!palette || !m_context)
			return;

		AssetHandle erasedTile = palette->GetTile(position);
		palette->EraseTile(position);

		if (m_context->tilemap.m_activeTile == erasedTile)
			m_context->tilemap.m_activeTile = 0;

		MarkPaletteDirty(m_context->tilemap.m_activePalette);
	}

	void TilePalettePanel::DrawActiveTilePreview()
	{
		auto assetManager = Project::GetEditorAssetManager();
		auto& tilemapState = m_context->tilemap;

		ImGui::TextUnformatted("Current Tile");

		if (tilemapState.m_activeTile == 0)
		{
			ImGui::TextDisabled("None");
			ImGui::Button("##EmptyTilePreview", ImVec2{ s_previewMaxSize, s_previewMaxSize });
			return;
		}

		ImGui::Text("%s", GetAssetLabel(assetManager, tilemapState.m_activeTile, AssetType::Tile).c_str());

		if (!assetManager || !assetManager->IsAssetHandleValid(tilemapState.m_activeTile)
			|| assetManager->GetAssetType(tilemapState.m_activeTile) != AssetType::Tile)
			return;

		Ref<TileAsset> tile = assetManager->GetAsset<TileAsset>(tilemapState.m_activeTile);
		if (!tile || tile->m_spriteHandle == 0)
		{
			ImGui::Button("##NoSpriteTilePreview", ImVec2{ s_previewMaxSize, s_previewMaxSize });
			return;
		}

		if (!assetManager->IsAssetHandleValid(tile->m_spriteHandle)
			|| assetManager->GetAssetType(tile->m_spriteHandle) != AssetType::Sprite)
		{
			ImGui::Button("##MissingSpriteTilePreview", ImVec2{ s_previewMaxSize, s_previewMaxSize });
			return;
		}

		Ref<SpriteAsset> sprite = assetManager->GetAsset<SpriteAsset>(tile->m_spriteHandle);
		if (!sprite || sprite->m_textureHandle == 0
			|| !assetManager->IsAssetHandleValid(sprite->m_textureHandle)
			|| assetManager->GetAssetType(sprite->m_textureHandle) != AssetType::Texture)
		{
			ImGui::Button("##MissingTextureTilePreview", ImVec2{ s_previewMaxSize, s_previewMaxSize });
			return;
		}

		Ref<Texture2D> texture = assetManager->GetAsset<Texture2D>(sprite->m_textureHandle);
		DrawTexturePreview(texture, sprite->m_uvMin, sprite->m_uvMax, tile->m_color);
	}

	AssetHandle TilePalettePanel::ResolveDroppedTile(const std::filesystem::path& filepath)
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || filepath.empty())
			return 0;

		AssetType assetType = assetManager->GetAssetTypeFromPath(filepath);
		switch (assetType)
		{
			case AssetType::Tile:
			{
				AssetHandle tileHandle = assetManager->ImportAsset(filepath);
				if (tileHandle != 0 && assetManager->GetAssetType(tileHandle) == AssetType::Tile)
					return tileHandle;
				break;
			}
			case AssetType::Sprite:
				return ResolveTileFromSprite(filepath);
			case AssetType::Texture:
				return ResolveTileFromTexture(filepath);
			default:
				break;
		}

		return 0;
	}

	AssetHandle TilePalettePanel::ResolveTileFromTexture(const std::filesystem::path& filepath)
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || !m_assetWorkflow)
			return 0;

		AssetHandle textureHandle = assetManager->ImportAsset(filepath);
		if (textureHandle == 0)
			return 0;

		AssetHandle spriteHandle = FindSpriteForTexture(textureHandle);
		if (spriteHandle == 0)
			spriteHandle = m_assetWorkflow->CreateSpriteFile(filepath.parent_path(), filepath.stem().string(), textureHandle);

		if (spriteHandle == 0)
			return 0;

		AssetHandle tileHandle = FindTileForSprite(spriteHandle);
		if (tileHandle == 0)
			tileHandle = m_assetWorkflow->CreateTileFile(filepath.parent_path(), filepath.stem().string(), spriteHandle);

		return tileHandle;
	}

	AssetHandle TilePalettePanel::ResolveTileFromSprite(const std::filesystem::path& filepath)
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || !m_assetWorkflow)
			return 0;

		AssetHandle spriteHandle = assetManager->ImportAsset(filepath);
		if (spriteHandle == 0 || assetManager->GetAssetType(spriteHandle) != AssetType::Sprite)
			return 0;

		AssetHandle tileHandle = FindTileForSprite(spriteHandle);
		if (tileHandle == 0)
			tileHandle = m_assetWorkflow->CreateTileFile(filepath.parent_path(), filepath.stem().string(), spriteHandle);

		return tileHandle;
	}

	AssetHandle TilePalettePanel::FindSpriteForTexture(AssetHandle textureHandle) const
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || textureHandle == 0)
			return 0;

		auto spriteHandles = assetManager->GetAllAssetsWithType(AssetType::Sprite);
		for (AssetHandle spriteHandle : spriteHandles)
		{
			Ref<SpriteAsset> sprite = assetManager->GetAsset<SpriteAsset>(spriteHandle);
			if (sprite && sprite->m_textureHandle == textureHandle)
				return spriteHandle;
		}

		return 0;
	}

	AssetHandle TilePalettePanel::FindTileForSprite(AssetHandle spriteHandle) const
	{
		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || spriteHandle == 0)
			return 0;

		auto tileHandles = assetManager->GetAllAssetsWithType(AssetType::Tile);
		for (AssetHandle tileHandle : tileHandles)
		{
			Ref<TileAsset> tile = assetManager->GetAsset<TileAsset>(tileHandle);
			if (tile && tile->m_spriteHandle == spriteHandle)
				return tileHandle;
		}

		return 0;
	}

	bool TilePalettePanel::SaveActivePalette()
	{
		if (!m_context)
			return false;

		AssetHandle paletteHandle = m_context->tilemap.m_activePalette;
		if (paletteHandle == 0)
			return false;

		auto assetManager = Project::GetEditorAssetManager();
		if (!assetManager || !assetManager->IsAssetHandleValid(paletteHandle)
			|| assetManager->GetAssetType(paletteHandle) != AssetType::TilePalette)
			return false;

		Ref<TilePaletteAsset> palette = assetManager->GetAsset<TilePaletteAsset>(paletteHandle);
		if (!palette)
			return false;

		const AssetMetadata& metadata = assetManager->GetMetadata(paletteHandle);
		if (!metadata.IsValid())
			return false;

		AssetImporter::Serialize(metadata, palette);
		assetManager->ReloadData(paletteHandle);
		m_dirtyPalettes.erase(paletteHandle);
		return true;
	}

	bool TilePalettePanel::IsActivePaletteDirty() const
	{
		if (!m_context)
			return false;

		return m_dirtyPalettes.find(m_context->tilemap.m_activePalette) != m_dirtyPalettes.end();
	}

	void TilePalettePanel::MarkPaletteDirty(AssetHandle paletteHandle)
	{
		if (paletteHandle != 0)
			m_dirtyPalettes.insert(paletteHandle);
	}

	void TilePalettePanel::CreateNewPalette()
	{
		if (!m_context || !m_assetWorkflow || !Project::GetActive())
			return;

		AssetHandle handle = m_assetWorkflow->CreateTilePaletteFile(Project::GetActiveAssetDirectory(), "New Palette");
		if (handle == 0)
			return;

		m_context->tilemap.m_activePalette = handle;
		m_context->tilemap.m_activeTile = 0;
	}

}
