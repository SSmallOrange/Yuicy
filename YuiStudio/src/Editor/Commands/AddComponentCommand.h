#pragma once

#include "../EditorCommand.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	// 添加组件命令
	template<typename T>
	class AddComponentCommand : public IEditorCommand
	{
	public:
		AddComponentCommand(Scene* scene, UUID entityUUID)
			: m_scene(scene), m_entityUUID(entityUUID)
		{
		}

		void Execute() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (entity && !entity.HasComponent<T>())
				entity.AddComponent<T>();
		}

		void Undo() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (entity && entity.HasComponent<T>())
				entity.RemoveComponent<T>();
		}

		std::string GetName() const override { return "Add Component"; }

	private:
		Scene* m_scene = nullptr;
		UUID m_entityUUID;
	};

}
