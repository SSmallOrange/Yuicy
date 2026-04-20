#include "SceneHierarchyPanel.h"

#include "Yuicy/Scene/Components.h"

#include "../Editor/Commands/CreateEntityCommand.h"
#include "../Editor/Commands/DeleteEntityCommand.h"
#include "../Editor/Commands/ReparentEntityCommand.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace Yuicy {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
	{
		SetContext(scene);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_context = scene;
		if (m_editorSelection)
			m_editorSelection->ClearEntitySelection();
	}

	Entity SceneHierarchyPanel::GetSelectedEntity() const
	{
		if (!m_editorSelection || !m_context)
			return {};

		UUID selectedUUID = m_editorSelection->GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0)
			return {};

		return m_context->FindEntityByUUID(selectedUUID);
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		if (!m_editorSelection)
			return;

		if (entity)
			m_editorSelection->SetSelectedEntity(entity.GetUUID());
		else
			m_editorSelection->ClearEntitySelection();
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		// 场景层级面板
		ImGui::Begin("Scene Hierarchy");

		if (m_context)
		{
			// 遍历所有实体，只绘制根实体
			auto view = m_context->GetAllEntitiesWith<IDComponent, RelationshipComponent>();
			for (auto entityID : view)
			{
				Entity entity{ entityID, m_context.get() };

				if (entity.GetParentUUID() == 0)
					DrawEntityNode(entity);
			}

			// 点击空白处取消选择
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				SetSelectedEntity({});

			// 右键空白处弹出菜单
			if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
				{
					if (m_commandHistory)
						m_commandHistory->ExecuteCommandT<CreateEntityCommand>(m_context.get(), "Empty Entity");
					else
					{
						m_context->CreateEntity("Empty Entity");
						if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
					}
				}

				if (ImGui::MenuItem("Create Camera"))
				{
					// 先创建实体，再添加组件（组合操作，作为单次创建处理）
					auto entity = m_context->CreateEntity("Camera");
					entity.AddComponent<CameraComponent>();
					if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
				}

				if (ImGui::MenuItem("Create Sprite"))
				{
					auto entity = m_context->CreateEntity("Sprite");
					entity.AddComponent<SpriteRendererComponent>();
					if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
				}

				ImGui::EndPopup();
			}

			// 拖拽到窗口空白区域脱离父级
			ImGuiWindow* window = ImGui::GetCurrentWindow();
			if (ImGui::BeginDragDropTargetCustom(window->ContentRegionRect, window->ID))
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DND"))
				{
					UUID droppedUUID = *(const UUID*)payload->Data;
					Entity droppedEntity = m_context->FindEntityByUUID(droppedUUID);

					if (droppedEntity && droppedEntity.GetParentUUID() != 0)
					{
						if (m_commandHistory)
							m_commandHistory->ExecuteCommandT<ReparentEntityCommand>(m_context.get(), droppedUUID, UUID(0));
						else
						{
							m_context->UnparentEntity(droppedEntity);
							if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		ImGui::End(); // Scene Hierarchy
	}

	// 实体树节点
	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& children = entity.Children();

		Entity selectedEntity = GetSelectedEntity();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		if (selectedEntity == entity)
			flags |= ImGuiTreeNodeFlags_Selected;

		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());

		if (ImGui::IsItemClicked())
			SetSelectedEntity(entity);

		// 右键菜单
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Child Entity"))
			{
				if (m_commandHistory)
					m_commandHistory->ExecuteCommandT<CreateChildEntityCommand>(m_context.get(), entity.GetUUID(), "Child Entity");
				else
				{
					m_context->CreateChildEntity(entity, "Child Entity");
					if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
				}
			}

			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		// 拖拽排序：设置拖拽源
		if (ImGui::BeginDragDropSource())
		{
			UUID uuid = entity.GetUUID();
			YUICY_CORE_INFO("Begin droppedUUID: {}");
			ImGui::SetDragDropPayload("ENTITY_DND", &uuid, sizeof(UUID));
			ImGui::Text("%s", tag.c_str());
			ImGui::EndDragDropSource();
		}

		// 拖拽排序：设置拖拽目标
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DND"))
			{
				UUID droppedUUID = *(const UUID*)payload->Data;
				Entity droppedEntity = m_context->FindEntityByUUID(droppedUUID);
				YUICY_CORE_INFO("End droppedUUID: {}");
				if (droppedEntity && droppedEntity != entity)
				{
					if (m_commandHistory)
						m_commandHistory->ExecuteCommandT<ReparentEntityCommand>(m_context.get(), droppedUUID, entity.GetUUID());
					else
					{
						m_context->ParentEntity(droppedEntity, entity);
						if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		// 递归绘制子节点
		if (opened)
		{
			for (auto& childUUID : children)
			{
				Entity child = m_context->FindEntityByUUID(childUUID);
				if (child)
					DrawEntityNode(child);
			}
			ImGui::TreePop();
		}

		// 延迟删除
		if (entityDeleted)
		{
			if (selectedEntity == entity)
				SetSelectedEntity({});

			if (m_commandHistory)
				m_commandHistory->ExecuteCommandT<DeleteEntityCommand>(m_context.get(), entity);
			else
			{
				m_context->DestroyEntity(entity);
				if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
			}
		}
	}

}
