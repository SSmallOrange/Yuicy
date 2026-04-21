#include "pch.h"

#include "SceneHierarchyPanel.h"

#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Scene/Components.h"

#include "../Editor/EditorCommandHistory.h"
#include "../Editor/EditorContext.h"
#include "../Editor/EditorDirtyTracker.h"
#include "../Editor/EditorSelectionContext.h"
#include "../Editor/Commands/CreateEntityCommand.h"
#include "../Editor/Commands/DeleteEntityCommand.h"
#include "../Editor/Commands/ReparentEntityCommand.h"
#include "../Utils/EditorIconUtils.h"

namespace Yuicy {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
	{
		SetContext(scene);
	}

	void SceneHierarchyPanel::Init()
	{
		m_hideIcon = EditorIconUtils::LoadIconTexture(
			"assets/textures/Editor/SceneHierarchyPanel/Hide.png", { 200, 200, 200, 255 });
		m_lockIcon = EditorIconUtils::LoadIconTexture(
			"assets/textures/Editor/SceneHierarchyPanel/lock.png", { 200, 200, 200, 255 });
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
		if (m_editorSelection)
		{
			if (entity)
				m_editorSelection->SetSelectedEntity(entity.GetUUID());
			else
				m_editorSelection->ClearEntitySelection();
		}
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		if (!m_context)
			return;

		ImGui::Begin("Scene Hierarchy");

		{
			// 遍历所有根实体
			auto view = m_context->GetAllEntitiesWith<TagComponent, RelationshipComponent>();
			for (auto entityHandle : view)
			{
				Entity entity(entityHandle, m_context.get());
				if (entity.GetParentUUID() == 0)
					DrawEntityNode(entity);
			}

			// 点击空白处取消选择
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered())
			{
				bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
				bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);
				if (!ctrl && !shift)
					SetSelectedEntity({});
			}

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
				ImGui::EndPopup();
			}

			// 拖拽到空白处 - Unparent
			if (ImGui::BeginDragDropTarget())
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
		UUID entityUUID = entity.GetUUID();

		// 获取锁定/隐藏状态
		bool isLocked = m_editorContext && m_editorContext->IsEntityLocked(entityUUID);
		bool isHidden = m_editorContext && m_editorContext->IsEntityHidden(entityUUID);

		// 隐藏实体变暗
		if (isHidden)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_FramePadding;

		if (m_editorSelection && m_editorSelection->IsEntitySelected(entityUUID))
			flags |= ImGuiTreeNodeFlags_Selected;

		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
		ImGui::PopStyleVar();

		// 保存树节点点击状态
		bool treeNodeClicked = ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen();

		// 右键菜单（此时 LastItem 仍是树节点）
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

		// 拖拽排序：设置拖拽源（此时 LastItem 仍是树节点）
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

		if (isHidden)
			ImGui::PopStyleColor();

		// 右侧锁定/隐藏图标按钮
		bool hideToggled = false;
		bool lockToggled = false;

		if (m_editorContext && m_hideIcon && m_lockIcon)
		{
			float iconSize = ImGui::GetFrameHeight() * 0.7f;
			float rowEndX = ImGui::GetWindowContentRegionMax().x;
			float btnStep = iconSize + 6.0f;

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

			// 隐藏切换
			ImGui::SameLine(rowEndX - btnStep * 2);
			ImVec4 hideTint = isHidden
				? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 0.3f);
			ImGui::PushID((int)(uint64_t)entityUUID + 1);
			if (ImGui::ImageButton(
				reinterpret_cast<ImTextureID>((uint64_t)m_hideIcon->GetRendererID()),
				ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), hideTint))
			{
				hideToggled = true;
			}
			ImGui::PopID();

			// 锁定切换
			ImGui::SameLine(rowEndX - btnStep);
			ImVec4 lockTint = isLocked
				? ImVec4(1.0f, 0.7f, 0.2f, 1.0f) : ImVec4(0.5f, 0.5f, 0.5f, 0.3f);
			ImGui::PushID((int)(uint64_t)entityUUID + 2);
			if (ImGui::ImageButton(
				reinterpret_cast<ImTextureID>((uint64_t)m_lockIcon->GetRendererID()),
				ImVec2(iconSize, iconSize), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0), lockTint))
			{
				lockToggled = true;
			}
			ImGui::PopID();

			ImGui::PopStyleVar();
			ImGui::PopStyleColor(3);
		}

		// 处理按钮动作
		if (hideToggled)
			m_editorContext->ToggleEntityHidden(entityUUID);
		if (lockToggled)
			m_editorContext->ToggleEntityLocked(entityUUID);

		// 选择逻辑（仅在点击树节点区域、非按钮时触发）
		if (treeNodeClicked && !hideToggled && !lockToggled)
		{
			bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
			bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

			if (ctrl)
				m_editorSelection->ToggleEntity(entityUUID);
			else if (shift)
				m_editorSelection->AddEntity(entityUUID);
			else
				SetSelectedEntity(entity);
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
