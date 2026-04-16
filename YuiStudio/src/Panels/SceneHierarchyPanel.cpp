#include "SceneHierarchyPanel.h"

#include "Yuicy/Scene/Components.h"

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
					m_context->CreateEntity("Empty Entity");

				if (ImGui::MenuItem("Create Camera"))
				{
					auto entity = m_context->CreateEntity("Camera");
					entity.AddComponent<CameraComponent>();
				}

				if (ImGui::MenuItem("Create Sprite"))
				{
					auto entity = m_context->CreateEntity("Sprite");
					entity.AddComponent<SpriteRendererComponent>();
				}

				ImGui::EndPopup();
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
				m_context->CreateChildEntity(entity, "Child Entity");

			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		// 拖拽排序：设置拖拽源
		if (ImGui::BeginDragDropSource())
		{
			UUID uuid = entity.GetUUID();
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

				if (droppedEntity && droppedEntity != entity)
					m_context->ParentEntity(droppedEntity, entity);
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
			m_context->DestroyEntity(entity);
		}
	}

}
