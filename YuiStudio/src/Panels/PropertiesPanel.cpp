#include "pch.h"

#include "PropertiesPanel.h"

#include "Yuicy/Scene/Components.h"
#include "Yuicy/Asset/AssetManager.h"
#include "Yuicy/Asset/EditorAssetManager.h"

#include "../Editor/EditorCommandHistory.h"
#include "../Editor/EditorDirtyTracker.h"
#include "../Editor/EditorSelectionContext.h"
#include "../Editor/Commands/AddComponentCommand.h"
#include "../Editor/Commands/RemoveComponentCommand.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>

namespace Yuicy {

	Entity PropertiesPanel::GetSelectedEntity() const
	{
		if (!m_editorSelection || !m_context)
			return {};

		UUID selectedUUID = m_editorSelection->GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0)
			return {};

		return m_context->FindEntityByUUID(selectedUUID);
	}

	void PropertiesPanel::OnImGuiRender()
	{
		ImGui::Begin("Properties");

		Entity selectedEntity = GetSelectedEntity();
		if (selectedEntity)
		{
			if (m_editorSelection && m_editorSelection->IsMultiSelection())
			{
				ImGui::TextColored(ImVec4(0.7f, 0.85f, 1.0f, 1.0f), "%d entities selected",
					(int)m_editorSelection->GetSelectionCount());
				ImGui::Separator();
			}

			DrawComponents(selectedEntity);
		}

		ImGui::End();
	}

	// Vec3 编辑控件
	void PropertiesPanel::DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
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
		{
			values.x = resetValue;
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		}
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Y
		ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		}
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		ImGui::PopItemWidth();
		ImGui::SameLine();

		// Z
		ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		}
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
			if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();
		ImGui::Columns(1);

		ImGui::PopID();
	}

	// DrawComponent 模板
	template<typename T, typename UIFunction>
	static void DrawComponentUI(const std::string& name, Entity entity, UIFunction uiFunction,
		EditorDirtyTracker* dirtyTracker = nullptr, EditorCommandHistory* commandHistory = nullptr,
		bool canRemove = true)
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

		if (canRemove)
		{
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
				ImGui::OpenPopup("ComponentSettings");

			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
				{
					if (commandHistory)
					{
						UUID entityUUID = entity.GetUUID();
						commandHistory->ExecuteCommandT<RemoveComponentCommand<T>>(entity.GetScene(), entityUUID);
					}
					else
					{
						entity.RemoveComponent<T>();
						if (dirtyTracker) dirtyTracker->MarkSceneDirty();
					}
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
	void PropertiesPanel::DrawAddComponentEntry(const std::string& entryName)
	{
		Entity selectedEntity = GetSelectedEntity();
		if (selectedEntity && !selectedEntity.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				if (m_commandHistory)
				{
					UUID entityUUID = selectedEntity.GetUUID();
					m_commandHistory->ExecuteCommandT<AddComponentCommand<T>>(m_context.get(), entityUUID);
				}
				else
				{
					selectedEntity.AddComponent<T>();
					if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
				}
				ImGui::CloseCurrentPopup();
			}
		}
	}

	// DrawComponents
	void PropertiesPanel::DrawComponents(Entity entity)
	{
		// Tag
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			std::array<char, 256> buffer{};
			std::memcpy(buffer.data(), tag.c_str(), std::min(tag.size(), buffer.size() - 1));

			if (ImGui::InputText("##Tag", buffer.data(), buffer.size()))
			{
				tag = std::string(buffer.data());
				if (m_dirtyTracker) m_dirtyTracker->MarkSceneDirty();
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DrawAddComponentEntry<SpriteRendererComponent>("Sprite Renderer");
			DrawAddComponentEntry<AnimationComponent>("Animation");
			DrawAddComponentEntry<CameraComponent>("Camera");
			DrawAddComponentEntry<LuaScriptComponent>("Lua Script");
			DrawAddComponentEntry<Rigidbody2DComponent>("Rigidbody 2D");
			DrawAddComponentEntry<BoxCollider2DComponent>("Box Collider 2D");
			DrawAddComponentEntry<CircleCollider2DComponent>("Circle Collider 2D");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		auto* dt = m_dirtyTracker;
		auto* ch = m_commandHistory;

		// TransformComponent
		DrawComponentUI<TransformComponent>("Transform", entity, [this](auto& component) {
			DrawVec3Control("Position", component.Translation);

			glm::vec3 rotation = glm::degrees(component.Rotation);
			DrawVec3Control("Rotation", rotation);
			component.Rotation = glm::radians(rotation);

			DrawVec3Control("Scale", component.Scale, 1.0f);
		}, nullptr, nullptr, false);

		// SpriteRendererComponent
		DrawComponentUI<SpriteRendererComponent>("Sprite Renderer", entity, [this, dt](auto& component) {
			m_spriteEditor.Draw(component, dt);
		}, dt, ch);

		// AnimationComponent
		DrawComponentUI<AnimationComponent>("Animation", entity, [this, dt](auto& component) {
			m_animationEditor.Draw(component, dt);
		}, dt, ch);

		// CameraComponent
		DrawComponentUI<CameraComponent>("Camera", entity, [this, dt](auto& component) {
			m_cameraEditor.Draw(component, dt);
		}, dt, ch);

		// LuaScriptComponent
		DrawComponentUI<LuaScriptComponent>("Lua Script", entity, [dt](auto& component)
		{
			auto assetManager = Project::GetEditorAssetManager();

			// 脚本名称显示
			std::string scriptLabel = "None";
			bool hasScript = false;
			bool scriptMissing = false;

			if (component.ScriptHandle != 0 && assetManager)
			{
				const auto& metadata = assetManager->GetMetadata(component.ScriptHandle);
				if (metadata.IsValid())
				{
					scriptLabel = metadata.filePath.filename().string();
					hasScript = true;

					// 检查文件是否存在
					std::filesystem::path fullPath = EditorAssetManager::GetFileSystemPath(metadata);
					std::error_code ec;
					if (!std::filesystem::exists(fullPath, ec))
						scriptMissing = true;
				}
			else
			{
				scriptLabel = "Missing (invalid handle)";
				scriptMissing = true;
			}
			}

			// 丢失脚本红色高亮
			if (scriptMissing)
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));

			ImGui::Text("Script: %s", scriptLabel.c_str());

			if (scriptMissing)
				ImGui::PopStyleColor();

			// 拖拽接收区域
			ImGui::Button(hasScript ? scriptLabel.c_str() : "Drop Script Here", ImVec2(ImGui::GetContentRegionAvail().x, 0));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					const wchar_t* droppedPath = (const wchar_t*)payload->Data;
					std::filesystem::path filepath = droppedPath;

					if (assetManager && assetManager->GetAssetTypeFromPath(filepath) == AssetType::LuaScript)
					{
						AssetHandle handle = assetManager->ImportAsset(filepath);
						if (handle != 0)
						{
							component.ScriptHandle = handle;
							if (dt) dt->MarkSceneDirty();
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			// 清除按钮
			if (hasScript)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("X##ClearScript"))
				{
					component.ScriptHandle = 0;
					component.IsLoaded = false;
					if (dt) dt->MarkSceneDirty();
				}
			}

			// 加载状态
			ImGui::Text("Loaded: %s", component.IsLoaded ? "Yes" : "No");

			if (scriptMissing)
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "Script file is missing!");
		}, dt, ch);

		// Rigidbody2DComponent
		DrawComponentUI<Rigidbody2DComponent>("Rigidbody 2D", entity, [dt](auto& component)
		{
			const char* bodyTypeStrings[] = { "Static", "Dynamic", "Kinematic" };
			const char* currentBodyTypeString = bodyTypeStrings[(int)component.Type];

			if (ImGui::BeginCombo("Body Type", currentBodyTypeString))
			{
				for (int i = 0; i < 3; i++)
				{
					bool isSelected = currentBodyTypeString == bodyTypeStrings[i];
					if (ImGui::Selectable(bodyTypeStrings[i], isSelected))
					{
						component.Type = (Rigidbody2DComponent::BodyType)i;
						if (dt) dt->MarkSceneDirty();
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}

			if (ImGui::Checkbox("Fixed Rotation", &component.FixedRotation))
				if (dt) dt->MarkSceneDirty();
		}, dt, ch);

		// BoxCollider2DComponent
		DrawComponentUI<BoxCollider2DComponent>("Box Collider 2D", entity, [this, dt](auto& component)
		{
			m_colliderEditor.DrawBoxCollider(component, dt);
		}, dt, ch);

		// CircleCollider2DComponent
		DrawComponentUI<CircleCollider2DComponent>("Circle Collider 2D", entity, [this, dt](auto& component)
		{
			m_colliderEditor.DrawCircleCollider(component, dt);
		}, dt, ch);
	}

}
