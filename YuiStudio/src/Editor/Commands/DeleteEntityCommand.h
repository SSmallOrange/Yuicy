#pragma once

#include "../EditorCommand.h"
#include "EntitySnapshot.h"

namespace Yuicy {

	// 删除实体命令
	class DeleteEntityCommand : public IEditorCommand
	{
	public:
		DeleteEntityCommand(Scene* scene, Entity entity)
			: m_scene(scene)
		{
			// 递归捕获实体及其所有后代的快照
			EntitySnapshot::CaptureHierarchy(entity, scene, m_snapshots);
		}

		void Execute() override
		{
			if (m_snapshots.empty())
				return;

			// 删除根实体
			Entity entity = m_scene->FindEntityByUUID(m_snapshots[0].uuid);
			if (entity)
				m_scene->DestroyEntity(entity);
		}

		void Undo() override
		{
			// 按快照顺序恢复(按照快照产生时的顺序恢复，避免乱序)
			for (const auto& snapshot : m_snapshots)
				snapshot.Restore(m_scene);

			// 恢复父子关系
			for (const auto& snapshot : m_snapshots)
			{
				Entity entity = m_scene->FindEntityByUUID(snapshot.uuid);
				if (!entity)
					continue;

				// 恢复 Children 列表
				auto& rel = entity.GetComponent<RelationshipComponent>();
				rel.Children = snapshot.relationship.Children;

				// 如果有父实体，将自己加入父实体的 Children
				if (snapshot.relationship.ParentHandle != 0)
				{
					Entity parent = m_scene->FindEntityByUUID(snapshot.relationship.ParentHandle);
					if (parent)
					{
						auto& parentChildren = parent.Children();
						UUID uuid = snapshot.uuid;
						if (std::ranges::find(parentChildren, uuid) == parentChildren.end())
							parentChildren.push_back(uuid);
					}
				}
			}
		}

		std::string GetName() const override { return "Delete Entity"; }

	private:
		Scene* m_scene = nullptr;
		std::vector<EntitySnapshot> m_snapshots;
	};

}
