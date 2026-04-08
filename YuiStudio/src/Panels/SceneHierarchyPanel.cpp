#include "SceneHierarchyPanel.h"

#include "Yuicy/Scene/Components.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

namespace Yuicy {

	SceneHierarchyPanel::SceneHierarchyPanel(const Ref<Scene>& scene)
	{
		SetContext(scene);
	}

	void SceneHierarchyPanel::SetContext(const Ref<Scene>& scene)
	{
		m_context = scene;
		m_selectionContext = {};
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
				m_selectionContext = {};

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

		// 属性面板
		ImGui::Begin("Properties");

		if (m_selectionContext)
			DrawComponents(m_selectionContext);

		ImGui::End(); // Properties
	}

	// 实体树节点
	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& children = entity.Children();

		// TreeNode 标志
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
			| ImGuiTreeNodeFlags_SpanAvailWidth;

		// 如果当前实体被选中，高亮显示
		if (m_selectionContext == entity)
			flags |= ImGuiTreeNodeFlags_Selected;

		// 如果没有子节点，显示为叶子节点
		if (children.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		// 用 UUID 作为唯一 ID
		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());

		// 点击选中
		if (ImGui::IsItemClicked())
			m_selectionContext = entity;

		// 右键菜单
		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Create Child Entity"))
			{
				m_context->CreateChildEntity(entity, "Child Entity");
			}

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

		// 如果展开了，递归绘制子节点
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
			if (m_selectionContext == entity)
				m_selectionContext = {};
			m_context->DestroyEntity(entity);
		}
	}

	// Vec3 编辑控件
	void SceneHierarchyPanel::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text("%s", label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		// X
		ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Y
		ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Z
		ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);

		ImGui::PopID();
	}

	// DrawComponent
	template<typename T, typename UIFunction>
	static void DrawComponentUI(const std::string& name, Entity entity, UIFunction uiFunction, bool canRemove = true)
	{
		if (!entity.HasComponent<T>())
			return;

		const ImGuiTreeNodeFlags treeNodeFlags =
			ImGuiTreeNodeFlags_DefaultOpen
			| ImGuiTreeNodeFlags_AllowItemOverlap
			| ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_Framed
			| ImGuiTreeNodeFlags_FramePadding;

		auto& component = entity.GetComponent<T>();

		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;

		ImGui::Separator();
		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, "%s", name.c_str());
		ImGui::PopStyleVar();

		// "..." 按钮（删除组件）
		if (canRemove)
		{
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
				ImGui::OpenPopup("ComponentSettings");

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
				{
					entity.RemoveComponent<T>();
					ImGui::EndPopup();
					if (open) ImGui::TreePop();
					return;
				}
				ImGui::EndPopup();
			}
		}

		if (open)
		{
			uiFunction(component);
			ImGui::TreePop();
		}
	}

	// Add Component
	template<typename T>
	void SceneHierarchyPanel::DrawAddComponentEntry(const std::string& entryName)
	{
		if (!m_selectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_selectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

	// DrawComponents
	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		// Tag
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			std::array<char, 256> buffer{};
			std::memcpy(buffer.data(), tag.c_str(), std::min(tag.size(), buffer.size() - 1));

			if (ImGui::InputText("##Tag", buffer.data(), buffer.size()))
				tag = std::string(buffer.data());
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DrawAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
			DrawAddComponentEntry<CameraComponent>("Camera");
			DrawAddComponentEntry<LuaScriptComponent>("Lua Script");
			DrawAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
			DrawAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DrawAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		// TransformComponent
		DrawComponentUI<TransformComponent>("Transform", entity, [this](auto& component) {
			DrawVec3Control("Position", component.Translation);

			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);

			DrawVec3Control("Scale", component.Scale, 1.0f);
		}, false); // Transform 不可删除

		// SpriteRendererComponent
		DrawComponentUI<SpriteRendererComponent>("Sprite Renderer", entity, [](auto& component) {
			ImGui::ColorEdit4("Color", glm::value_ptr(component.Color));

			// 纹理路径
			std::array<char, 256> texPathBuffer{};
			std::memcpy(texPathBuffer.data(), component.TexturePath.c_str(), 
						std::min(component.TexturePath.size(), texPathBuffer.size() - 1));
			
			if (ImGui::InputText("Texture Path", texPathBuffer.data(), texPathBuffer.size()))
			{
				component.TexturePath = texPathBuffer.data();
				if (!component.TexturePath.empty() && std::filesystem::exists(component.TexturePath))
					component.Texture = Texture2D::Create(component.TexturePath);
			}

			ImGui::DragFloat("Tiling Factor", &component.TilingFactor, 0.1f, 0.0f, 100.0f);
			ImGui::Checkbox("Flip X", &component.FlipX);
			ImGui::SameLine();
			ImGui::Checkbox("Flip Y", &component.FlipY);
			ImGui::DragInt("Sorting Order", &component.SortingOrder);
		});

		// CameraComponent
		DrawComponentUI<CameraComponent>("Camera", entity, [](auto& component) {
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			float orthoSize = camera.GetOrthographicSize();
			if (ImGui::DragFloat("Orthographic Size", &orthoSize, 0.1f))
				camera.SetOrthographicSize(orthoSize);

			float orthoNear = camera.GetOrthographicNearClip();
			if (ImGui::DragFloat("Near Clip", &orthoNear, 0.1f))
				camera.SetOrthographicNearClip(orthoNear);

			float orthoFar = camera.GetOrthographicFarClip();
			if (ImGui::DragFloat("Far Clip", &orthoFar, 0.1f))
				camera.SetOrthographicFarClip(orthoFar);

			ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
		});

		// LuaScriptComponent
		DrawComponentUI<LuaScriptComponent>("Lua Script", entity, [](auto& component)
		{
			std::array<char, 256> scriptPathBuffer{};
			std::memcpy(scriptPathBuffer.data(), component.ScriptPath.c_str(), 
						std::min(component.ScriptPath.size(), scriptPathBuffer.size() - 1));
			if (ImGui::InputText("Script Path", scriptPathBuffer.data(), scriptPathBuffer.size()))
				component.ScriptPath = scriptPathBuffer.data();

			ImGui::Text("Loaded: %s", component.IsLoaded ? "Yes" : "No");
		});

		// Rigidbody2DComponent
		DrawComponentUI<Rigidbody2DComponent>("Rigidbody 2D", entity, [](auto& component)
		{
			const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];

			if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
						component.Type = (Rigidbody2DComponent::BodyType)i;

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			ImGui::Checkbox("Fixed Rotation", &component.FixedRotation);
		});

		// BoxCollider2DComponent
		DrawComponentUI<BoxCollider2DComponent>("Box Collider 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f);
			ImGui::DragFloat2("Size", glm::value_ptr(component.Size), 0.01f);
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
			ImGui::Checkbox("Is Trigger", &component.IsTrigger);
		});

		// CircleCollider2DComponent
		DrawComponentUI<CircleCollider2DComponent>("Circle Collider 2D", entity, [](auto& component)
		{
			ImGui::DragFloat2("Offset", glm::value_ptr(component.Offset), 0.01f);
			ImGui::DragFloat("Radius", &component.Radius, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Density", &component.Density, 0.01f, 0.0f, 10.0f);
			ImGui::DragFloat("Friction", &component.Friction, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution", &component.Restitution, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Restitution Threshold", &component.RestitutionThreshold, 0.01f, 0.0f);
			ImGui::Checkbox("Is Trigger", &component.IsTrigger);
		});
	}

}
