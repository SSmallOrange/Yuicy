#pragma once

#include "../EditorCommand.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"

namespace Yuicy {

	// 删除组件命令
	template<typename T>
	class RemoveComponentCommand : public IEditorCommand
	{
	public:
		RemoveComponentCommand(Scene* scene, UUID entityUUID)
			: m_scene(scene), m_entityUUID(entityUUID)
		{
			// 构造时捕获组件数据快照
			Entity entity = scene->FindEntityByUUID(entityUUID);
			if (entity && entity.HasComponent<T>())
				m_snapshot = entity.GetComponent<T>();
		}

		void Execute() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (entity && entity.HasComponent<T>())
				entity.RemoveComponent<T>();
		}

		void Undo() override
		{
			Entity entity = m_scene->FindEntityByUUID(m_entityUUID);
			if (entity && !entity.HasComponent<T>())
				entity.AddComponent<T>(m_snapshot);
		}

		std::string GetName() const override { return "Remove Component"; }

	private:
		Scene* m_scene = nullptr;
		UUID m_entityUUID;
		T m_snapshot{};
	};

}
