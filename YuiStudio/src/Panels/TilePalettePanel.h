#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <filesystem>
#include <unordered_set>

namespace Yuicy {

	struct EditorContext;
	class EditorAssetWorkflow;
	class EditorDirtyTracker;
	class Entity;
	struct TilePaletteAsset;

	class TilePalettePanel
	{
	public:
		TilePalettePanel() = default;
		~TilePalettePanel() = default;

		void SetContext(EditorContext* context) { m_context = context; }
		void SetAssetWorkflow(EditorAssetWorkflow* assetWorkflow) { m_assetWorkflow = assetWorkflow; }
		void SetDirtyTracker(EditorDirtyTracker* dirtyTracker) { m_dirtyTracker = dirtyTracker; }

		void OnImGuiRender(bool* open);

	private:
		bool IsProjectReady() const;

		void DrawProjectDisabledState();
		void DrawPaletteSelector();
		void DrawActiveTargetSelector();
		void DrawToolButtons();
		void DrawPaletteGrid();
		void DrawActiveTilePreview();
		Entity GetSelectedTilemapEntity() const;
		bool ActiveToolNeedsTarget() const;
		bool ActiveToolNeedsTile() const;
		void ValidateActiveAssets();

		void CreateNewPalette();
		bool SaveActivePalette();
		bool IsActivePaletteDirty() const;
		void MarkPaletteDirty(AssetHandle paletteHandle);

		void DrawPaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position);
		void SetPaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position, AssetHandle tileHandle);
		void ErasePaletteCell(const Ref<TilePaletteAsset>& palette, const GridPosition& position);

		AssetHandle ResolveDroppedTile(const std::filesystem::path& filepath);
		AssetHandle ResolveTileFromTexture(const std::filesystem::path& filepath);
		AssetHandle ResolveTileFromSprite(const std::filesystem::path& filepath);
		AssetHandle FindSpriteForTexture(AssetHandle textureHandle) const;
		AssetHandle FindTileForSprite(AssetHandle spriteHandle) const;

	private:
		EditorContext* m_context = nullptr;
		EditorAssetWorkflow* m_assetWorkflow = nullptr;
		EditorDirtyTracker* m_dirtyTracker = nullptr;
		std::unordered_set<AssetHandle> m_dirtyPalettes;
	};

}
