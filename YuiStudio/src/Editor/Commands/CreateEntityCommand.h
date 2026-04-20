#pragma once

#include "../EditorCommand.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	// 创建实体命令
	class CreateEntityCommand : public IEditorCommand
	{
	public:
		CreateEntityCommand(Scene* scene, const std::string& name)
			: m_scene(scene), m_name(name)
		{
		}

		void Execute() override
		{
			Entity entity = m_scene->CreateEntity(m_name);
			m_createdUUID = entity.GetUUID();
		}

		void Undo() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_createdUUID);
			if (entity)
				m_scene->DestroyEntity(entity);
		}

		std::string GetName() const override { return "Create Entity"; }

		UUID GetCreatedUUID() const { return m_createdUUID; }

	private:
		Scene* m_scene = nullptr;
		std::string m_name;
		UUID m_createdUUID = 0;
	};

	// 创建子实体命令
	class CreateChildEntityCommand : public IEditorCommand
	{
	public:
		CreateChildEntityCommand(Scene* scene, UUID parentUUID, const std::string& name)
			: m_scene(scene), m_parentUUID(parentUUID), m_name(name)
		{
		}

		void Execute() override
		{
			Entity parent = m_scene->FindEntityByUUID(m_parentUUID);
			if (parent)
			{
				Entity entity = m_scene->CreateChildEntity(parent, m_name);
				m_createdUUID = entity.GetUUID();
			}
		}

		void Undo() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_createdUUID);
			if (entity)
				m_scene->DestroyEntity(entity);
		}

		std::string GetName() const override { return "Create Child Entity"; }

		UUID GetCreatedUUID() const { return m_createdUUID; }

	private:
		Scene* m_scene = nullptr;
		UUID m_parentUUID;
		std::string m_name;
		UUID m_createdUUID = 0;
	};

}
