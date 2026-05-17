#pragma once

#include "../EditorCommand.h"

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

#include <optional>
#include <vector>

namespace Yuicy {

	class Scene;

	struct TileChange
	{
		GridPosition position;
		std::optional<TileCell> before;
		std::optional<TileCell> after;
	};

	class PaintTileBatchCommand : public IEditorCommand
	{
	public:
		PaintTileBatchCommand(Scene* scene, UUID tilemapEntityUUID, std::vector<TileChange> changes);

		void Execute() override;
		void Undo() override;

		std::string GetName() const override { return "Paint Tile Batch"; }
		std::string GetCommandID() const override { return "PaintTileBatchCommand"; }

		bool IsEmpty() const { return m_changes.empty(); }

	private:
		void Apply(bool useAfter);

	private:
		Scene* m_scene = nullptr;
		UUID m_tilemapEntityUUID = 0;
		std::vector<TileChange> m_changes;
	};

}
