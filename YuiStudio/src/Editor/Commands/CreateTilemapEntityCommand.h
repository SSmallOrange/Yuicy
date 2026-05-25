#pragma once

#include "../EditorCommand.h"
#include "../EditorSelectionContext.h"

#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	class CreateTilemapEntityCommand : public IEditorCommand
	{
	public:
		CreateTilemapEntityCommand(Scene* scene, EditorSelectionContext* selection = nullptr, UUID targetGridUUID = 0)
			: m_scene(scene), m_selection(selection)
		{
			if (targetGridUUID != 0)
			{
				m_gridUUID = targetGridUUID;
				m_reuseExistingGrid = true;
			}
		}

		void Execute() override
		{
			if (!m_scene)
				return;

			Entity gridEntity = m_scene->FindEntityByUUID(m_gridUUID);
			if (!gridEntity)
			{
				if (m_reuseExistingGrid)
					return;

				gridEntity = m_scene->CreateEntityWithUUID(m_gridUUID, "Grid");
				gridEntity.AddComponent<GridComponent>();
			}
			else if (!gridEntity.HasComponent<GridComponent>())
			{
				gridEntity.AddComponent<GridComponent>();
			}

			Entity tilemapEntity = m_scene->FindEntityByUUID(m_tilemapUUID);
			if (!tilemapEntity)
			{
				tilemapEntity = m_scene->CreateEntityWithUUID(m_tilemapUUID, "Tilemap");
				tilemapEntity.SetParent(gridEntity);
				tilemapEntity.AddComponent<TilemapComponent>();
				tilemapEntity.AddComponent<TilemapRendererComponent>();
			}
			else
			{
				tilemapEntity.SetParent(gridEntity);
				if (!tilemapEntity.HasComponent<TilemapComponent>())
					tilemapEntity.AddComponent<TilemapComponent>();
				if (!tilemapEntity.HasComponent<TilemapRendererComponent>())
					tilemapEntity.AddComponent<TilemapRendererComponent>();
			}

			SelectTilemap();
		}

		void Undo() override
		{
			if (!m_scene)
				return;

			Entity gridEntity = m_scene->FindEntityByUUID(m_gridUUID);
			if (m_reuseExistingGrid)
			{
				Entity tilemapEntity = m_scene->FindEntityByUUID(m_tilemapUUID);
				if (tilemapEntity)
					m_scene->DestroyEntity(tilemapEntity);
			}
			else if (gridEntity)
			{
				m_scene->DestroyEntity(gridEntity);
			}
			else
			{
				Entity tilemapEntity = m_scene->FindEntityByUUID(m_tilemapUUID);
				if (tilemapEntity)
					m_scene->DestroyEntity(tilemapEntity);
			}

			if (m_selection)
			{
				m_selection->RemoveEntity(m_tilemapUUID);
				m_selection->RemoveEntity(m_gridUUID);
			}
		}

		std::string GetName() const override { return "Create Tilemap"; }
		std::string GetCommandID() const override { return "CreateTilemapEntityCommand"; }

		UUID GetGridUUID() const { return m_gridUUID; }
		UUID GetTilemapUUID() const { return m_tilemapUUID; }

	private:
		void SelectTilemap()
		{
			if (!m_selection)
				return;

			m_selection->SetSelectedEntity(m_tilemapUUID);
			m_selection->ClearAssetSelection();
		}

	private:
		Scene* m_scene = nullptr;
		EditorSelectionContext* m_selection = nullptr;
		UUID m_gridUUID;
		UUID m_tilemapUUID;
		bool m_reuseExistingGrid = false;
	};

}
