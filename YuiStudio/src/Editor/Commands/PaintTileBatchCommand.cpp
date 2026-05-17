#include "pch.h"

#include "PaintTileBatchCommand.h"

#include "Yuicy/Scene/Components.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy {

	PaintTileBatchCommand::PaintTileBatchCommand(Scene* scene, UUID tilemapEntityUUID, std::vector<TileChange> changes)
		: m_scene(scene), m_tilemapEntityUUID(tilemapEntityUUID), m_changes(std::move(changes))
	{
	}

	void PaintTileBatchCommand::Execute()
	{
		Apply(true);
	}

	void PaintTileBatchCommand::Undo()
	{
		Apply(false);
	}

	void PaintTileBatchCommand::Apply(bool useAfter)
	{
		if (!m_scene || m_tilemapEntityUUID == 0)
			return;

		Entity tilemapEntity = m_scene->FindEntityByUUID(m_tilemapEntityUUID);
		if (!tilemapEntity || !tilemapEntity.HasComponent<TilemapComponent>())
			return;

		auto& tilemap = tilemapEntity.GetComponent<TilemapComponent>();
		for (const TileChange& change : m_changes)
		{
			const std::optional<TileCell>& target = useAfter ? change.after : change.before;
			if (target)
				tilemap.SetTile(change.position, *target);
			else
				tilemap.EraseTile(change.position);
		}
	}

}
