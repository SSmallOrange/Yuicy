#pragma once

#include "../EditorCommand.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	// 重新设置父实体命令
	class ReparentEntityCommand : public IEditorCommand
	{
	public:
		ReparentEntityCommand(Scene* scene, UUID entityUUID, UUID newParentUUID)
			: m_scene(scene), m_entityUUID(entityUUID), m_newParentUUID(newParentUUID)
		{
			Entity entity = scene->FindEntityByUUID(entityUUID);
			if (entity)
				m_oldParentUUID = entity.GetParentUUID();
		}

		void Execute() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (!entity)
				return;

			if (m_newParentUUID == 0)
			{
				m_scene->UnparentEntity(entity);
			}
			else
			{
				Entity newParent = m_scene->FindEntityByUUID(m_newParentUUID);
				if (newParent)
					m_scene->ParentEntity(entity, newParent);
			}
		}

		void Undo() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (!entity)
				return;

			// 先解除当前父子关系
			m_scene->UnparentEntity(entity);

			// 恢复旧的父子关系
			if (m_oldParentUUID != 0)
			{
				Entity oldParent = m_scene->FindEntityByUUID(m_oldParentUUID);
				if (oldParent)
					m_scene->ParentEntity(entity, oldParent);
			}
		}

		std::string GetName() const override { return "Reparent Entity"; }

	private:
		Scene* m_scene = nullptr;
		UUID m_entityUUID;
		UUID m_oldParentUUID = 0;
		UUID m_newParentUUID;
	};

}
