#pragma once

#include "../EditorCommand.h"
#include "../EditorSelectionContext.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	// 创建实体命令
	class CreateEntityCommand : public IEditorCommand
	{
	public:
		CreateEntityCommand(Scene* scene, const std::string& name, EditorSelectionContext* selection = nullptr)
			: m_scene(scene), m_name(name), m_selection(selection)
		{
		}

		void Execute() override
		{
			if (!m_scene || m_scene->FindEntityByUUID(m_createdUUID))
				return;

			m_scene->CreateEntityWithUUID(m_createdUUID, m_name);
			SelectEntity();
		}

		void Undo() override
		{
			if (!m_scene)
				return;

			Entity entity = m_scene->FindEntityByUUID(m_createdUUID);
			if (entity)
				m_scene->DestroyEntity(entity);

			if (m_selection)
				m_selection->RemoveEntity(m_createdUUID);
		}

		std::string GetName() const override { return "Create Entity"; }

		UUID GetCreatedUUID() const { return m_createdUUID; }

	private:
		void SelectEntity()
		{
			if (!m_selection)
				return;

			m_selection->SetSelectedEntity(m_createdUUID);
			m_selection->ClearAssetSelection();
		}

	private:
		Scene* m_scene = nullptr;
		std::string m_name;
		EditorSelectionContext* m_selection = nullptr;
		UUID m_createdUUID;
	};

	// 创建子实体命令
	class CreateChildEntityCommand : public IEditorCommand
	{
	public:
		CreateChildEntityCommand(Scene* scene, UUID parentUUID, const std::string& name, EditorSelectionContext* selection = nullptr)
			: m_scene(scene), m_parentUUID(parentUUID), m_name(name), m_selection(selection)
		{
		}

		void Execute() override
		{
			if (!m_scene || m_scene->FindEntityByUUID(m_createdUUID))
				return;

			Entity parent = m_scene->FindEntityByUUID(m_parentUUID);
			if (parent)
			{
				Entity entity = m_scene->CreateEntityWithUUID(m_createdUUID, m_name);
				entity.SetParent(parent);
				SelectEntity();
			}
		}

		void Undo() override
		{
			if (!m_scene)
				return;

			Entity entity = m_scene->FindEntityByUUID(m_createdUUID);
			if (entity)
				m_scene->DestroyEntity(entity);

			if (m_selection)
				m_selection->RemoveEntity(m_createdUUID);
		}

		std::string GetName() const override { return "Create Child Entity"; }

		UUID GetCreatedUUID() const { return m_createdUUID; }

	private:
		void SelectEntity()
		{
			if (!m_selection)
				return;

			m_selection->SetSelectedEntity(m_createdUUID);
			m_selection->ClearAssetSelection();
		}

	private:
		Scene* m_scene = nullptr;
		UUID m_parentUUID;
		std::string m_name;
		EditorSelectionContext* m_selection = nullptr;
		UUID m_createdUUID;
	};

}
