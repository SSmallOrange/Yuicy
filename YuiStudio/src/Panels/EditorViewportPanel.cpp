#include "pch.h"

#include "EditorViewportPanel.h"

#include "../Editor/Commands/SetTransformCommand.h"
#include "../Editor/EditorCommandHistory.h"
#include "../Editor/EditorContext.h"
#include "../Editor/EditorDirtyTracker.h"
#include "../Editor/EditorRenderPipeline.h"
#include "../Editor/EditorSceneController.h"

#include "../Utils/EditorIconUtils.h"

#include "Yuicy/Events/Event.h"
#include "Yuicy/Events/KeyEvent.h"
#include "Yuicy/Events/MouseEvent.h"
#include "Yuicy/Core/Input.h"
#include "Yuicy/Core/KeyCodes.h"
#include "Yuicy/Core/MouseCodes.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Scene/SceneSerializer.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>

namespace Yuicy {

	void EditorViewportPanel::Init()
	{
		m_editorCamera = EditorCamera(1280.0f / 720.0f, 5.0f);

		m_playIcon			= EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Play.png",  { 50, 200, 50, 255 });
		m_simulateIcon		= EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/debug.png", { 50, 150, 200, 255 });
		m_stopIcon			= EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Stop.png",  { 200, 50, 50, 255 });
		m_pauseStartIcon    = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Pause-Stop.png", { 200, 200, 50, 255 });
		m_pauseStopIcon     = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Pause-Start.png", { 200, 200, 50, 255 });
		m_stepIcon			= EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Step-Forward.png",  { 50, 200, 200, 255 });
		m_overlayIcon		= EditorIconUtils::LoadIconTexture("assets/textures/Editor/Generic/Gear.png",   { 160, 160, 160, 255 });
	}

	void EditorViewportPanel::OnSceneChanged()
	{
		m_gizmoType = -1;
	}

	void EditorViewportPanel::OnUpdate(Timestep ts)
	{
		if (!m_context || !m_renderPipeline)
			return;

		auto& viewportState = m_context->viewport;

		// Viewport resize
		auto fbSpec = m_renderPipeline->GetFramebuffer()->GetSpecification();
		if (viewportState.size.x > 0.0f && viewportState.size.y > 0.0f
			&& (fbSpec.width != (uint32_t)viewportState.size.x || fbSpec.height != (uint32_t)viewportState.size.y))
		{
			uint32_t width = (uint32_t)viewportState.size.x;
			uint32_t height = (uint32_t)viewportState.size.y;
			m_renderPipeline->OnViewportResize(width, height);
			m_context->activeScene->OnViewportResize(width, height);
			m_editorCamera.SetViewportSize(width, height);
		}

		// 传递视口相对鼠标位置给编辑器相机（用于缩放到鼠标 / MMB 平移）
		auto [globalMouseX, globalMouseY] = ImGui::GetMousePos();
		float viewportMouseX = globalMouseX - viewportState.bounds[0].x;
		float viewportMouseY = globalMouseY - viewportState.bounds[0].y;
		m_editorCamera.SetViewportMousePosition(viewportMouseX, viewportMouseY);

		// 编辑器相机更新（悬停或正在 MMB 平移时允许更新）
		if (m_context->runtime.mode != SceneMode::Play && (viewportState.hovered || m_editorCamera.IsPanning()))
			m_editorCamera.OnUpdate(ts);

		// 执行渲染管线
		m_renderPipeline->Execute(ts, m_editorCamera);

		// 框选逻辑更新
		UpdateBoxSelection();

		// 鼠标拾取
		UpdateMousePicking();
	}

	void EditorViewportPanel::UpdateMousePicking()
	{
		auto& viewportState = m_context->viewport;

		auto [mx, my] = ImGui::GetMousePos();
		mx -= viewportState.bounds[0].x;
		my -= viewportState.bounds[0].y;

		glm::vec2 viewportBoundsSize = viewportState.bounds[1] - viewportState.bounds[0];
		my = viewportBoundsSize.y - my;

		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0
			&& mouseX < (int)viewportBoundsSize.x && mouseY < (int)viewportBoundsSize.y)
		{
			int pixelData = m_renderPipeline->ReadEntityIDAtPixel(mouseX, mouseY);
			viewportState.hoveredEntity = pixelData == -1 ? Entity{} : Entity((entt::entity)pixelData, m_context->activeScene.get());
		}
		else
		{
			viewportState.hoveredEntity = {};
		}
	}

	void EditorViewportPanel::OnImGuiRender()
	{
		if (!m_context || !m_renderPipeline)
			return;

		auto& viewportState = m_context->viewport;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		viewportState.focused = ImGui::IsWindowFocused();
		viewportState.hovered = ImGui::IsWindowHovered();

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		viewportState.size = { viewportPanelSize.x, viewportPanelSize.y };

		// 计算 Viewport 的屏幕坐标
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportOffset = ImGui::GetWindowPos();
		viewportState.bounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		viewportState.bounds[1] = viewportState.bounds[0] + viewportState.size;

		uint64_t textureID = m_renderPipeline->GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ viewportState.size.x, viewportState.size.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		// 接收从 ContentBrowser 拖拽过来的场景文件
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const auto* pathData = (const std::filesystem::path::value_type*)payload->Data;
				std::filesystem::path filepath(pathData);
				if (filepath.extension() == SceneSerializer::GetSceneSerializerDefaultExtension())
					m_sceneController->OpenScene(filepath);
			}

			ImGui::EndDragDropTarget();
		}

		// Gizmo 绘制
		OnImGuiDrawGizmos();

		// 框选矩形 Overlay
		if (m_isBoxSelecting)
			DrawBoxSelectionOverlay();

		ImGui::End(); // Viewport
		ImGui::PopStyleVar();
		
		// Setting Render
		OnImGuiOverlaySettingsRender();

		// Play/Stop 工具栏
		OnImGuiToolbarRender();
	}

	void EditorViewportPanel::OnImGuiDrawGizmos()
	{
		if (!m_context->runtime.IsEditing())
			return;

		// 从共享选择上下文解析选中实体
		UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0 || m_gizmoType == -1)
			return;

		Entity selectedEntity = m_context->activeScene->FindEntityByUUID(selectedUUID);
		if (!selectedEntity)
			return;

		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(
			m_context->viewport.bounds[0].x, m_context->viewport.bounds[0].y,
			m_context->viewport.size.x, m_context->viewport.size.y);

		const glm::mat4& cameraProjection = m_editorCamera.GetProjection();
		glm::mat4 cameraView = m_editorCamera.GetViewMatrix();
		glm::mat4 worldTransform = m_context->activeScene->GetWorldSpaceTransformMatrix(selectedEntity);

		// 吸附：根据 Gizmo 操作类型从 EditorViewportSettings 读取吸附参数
		// Ctrl 键作为吸附切换：设置中开启则 Ctrl 临时关闭，设置中关闭则 Ctrl 临时开启
		auto& snapSettings = m_context->viewportSettings;
		bool ctrlPressed = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);

		bool snapEnabled = false;
		float snapValue = 0.0f;

		switch (m_gizmoType)
		{
		case ImGuizmo::OPERATION::TRANSLATE:
			snapEnabled = snapSettings.enableTranslationSnap;
			snapValue = snapSettings.translationSnapValue;
			break;
		case ImGuizmo::OPERATION::ROTATE:
			snapEnabled = snapSettings.enableRotationSnap;
			snapValue = snapSettings.rotationSnapValue;
			break;
		case ImGuizmo::OPERATION::SCALE:
			snapEnabled = snapSettings.enableScaleSnap;
			snapValue = snapSettings.scaleSnapValue;
			break;
		}

		// Ctrl 切换吸附状态
		bool snap = ctrlPressed ? !snapEnabled : snapEnabled;
		float snapValues[3] = { snapValue, snapValue, snapValue };

		ImGuizmo::Manipulate(
			glm::value_ptr(cameraView),
			glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_gizmoType,
			ImGuizmo::LOCAL,
			glm::value_ptr(worldTransform),
			nullptr,
			snap ? snapValues : nullptr
		);

		bool wasUsing = m_gizmoInUse;
		m_gizmoInUse = ImGuizmo::IsUsing();

		// Gizmo 拖拽开始：捕获初始 Transform 快照
		if (m_gizmoInUse && !wasUsing)
		{
			m_gizmoOldTransform = selectedEntity.GetComponent<TransformComponent>();
			m_gizmoSnapshotCaptured = true;
		}

		// Gizmo 拖拽中：应用 Transform 变换
		if (m_gizmoInUse)
		{
			glm::mat4 localTransform = worldTransform;
			Entity parent = selectedEntity.GetParent();
			if (parent)
			{
				glm::mat4 parentWorldTransform = m_context->activeScene->GetWorldSpaceTransformMatrix(parent);
				localTransform = glm::inverse(parentWorldTransform) * worldTransform;
			}

			selectedEntity.GetComponent<TransformComponent>().SetTransform(localTransform);
		}

		// Gizmo 拖拽结束：提交 SetTransformCommand
		if (wasUsing && !m_gizmoInUse && m_gizmoSnapshotCaptured)
		{
			m_gizmoSnapshotCaptured = false;

			auto& tc = selectedEntity.GetComponent<TransformComponent>();
			if (m_commandHistory)
			{
				UUID entityUUID = selectedEntity.GetUUID();
				auto cmd = CreateScope<SetTransformCommand>(
					m_context->activeScene.get(), entityUUID,
					m_gizmoOldTransform.Translation, m_gizmoOldTransform.Rotation, m_gizmoOldTransform.Scale,
					tc.Translation, tc.Rotation, tc.Scale
				);
				// Transform 已在拖拽中实时应用
				// 直接压栈，跳过 Execute（会多执行一次，但是无害）
				m_commandHistory->ExecuteCommand(std::move(cmd));
			}
			else if (m_dirtyTracker)
			{
				m_dirtyTracker->MarkSceneDirty();
			}
		}
	}

	void EditorViewportPanel::OnImGuiToolbarRender()
	{
		auto& viewportState = m_context->viewport;

		if (viewportState.size.x <= 0.0f || viewportState.size.y <= 0.0f)
			return;

		auto& runtime = m_context->runtime;
		bool isEditing = runtime.IsEditing();
		bool isRunning = runtime.IsRunning();

		const float edgeOffset = 8.0f;
		const float iconSize = 24.0f;
		const float buttonSpacing = 4.0f;

		// Edit: [Play] [Simulate]  |  Running: [Stop] [Pause] [Step]
		int buttonCount = isEditing ? 2 : 3;
		float contentWidth = iconSize * buttonCount + buttonSpacing * (buttonCount - 1);
		float windowWidth = contentWidth + edgeOffset * 2.0f;
		float windowHeight = iconSize + edgeOffset;

		float toolbarX = (viewportState.bounds[0].x + viewportState.bounds[1].x) * 0.5f;
		ImGui::SetNextWindowPos(ImVec2(toolbarX - windowWidth * 0.5f, viewportState.bounds[0].y + edgeOffset));
		ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
		ImGui::SetNextWindowBgAlpha(0.75f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(edgeOffset, edgeOffset * 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(buttonSpacing, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::Begin("##toolbar", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

		auto drawToolbarButton = [&](const char* id, Ref<Texture2D> icon, auto onClickFn)
		{
			ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)icon->GetRendererID());
			if (ImGui::ImageButton(id, texID, ImVec2{ iconSize, iconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
				onClickFn();
		};

		if (isEditing)
		{
			// [Play] [Simulate]
			drawToolbarButton("##Play", m_playIcon, [this]() { m_sceneController->OnScenePlay(); });
			ImGui::SameLine();
			drawToolbarButton("##Simulate", m_simulateIcon, [this]() { m_sceneController->OnSceneSimulate(); });
		}
		else
		{
			// [Stop] [Pause/Resume] [Step]
			drawToolbarButton("##Stop", m_stopIcon, [this]() { m_sceneController->OnSceneStop(); });
			ImGui::SameLine();

			Ref<Texture2D> pauseIcon = runtime.paused ? m_pauseStopIcon : m_pauseStartIcon;
			drawToolbarButton("##Pause", pauseIcon, [this]() { m_sceneController->OnScenePause(); });
			ImGui::SameLine();

			drawToolbarButton("##Step", m_stepIcon, [this]() { m_sceneController->OnSceneStep(); });
		}

		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();  // FramePadding
		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(4);
	}

	void EditorViewportPanel::OnImGuiOverlaySettingsRender()
	{
		auto& viewportState = m_context->viewport;
		if (viewportState.size.x <= 0.0f || viewportState.size.y <= 0.0f)
			return;

		const float edgeOffset = 8.0f;
		const float iconSize = 18.0f;

		ImGui::SetNextWindowPos(
			ImVec2(viewportState.bounds[1].x - edgeOffset, viewportState.bounds[0].y + edgeOffset),
			ImGuiCond_Always,
			ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.75f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::Begin("##viewport_overlay_controls", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
			| ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));

		ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)m_overlayIcon->GetRendererID());
		if (ImGui::ImageButton("##OverlaySettings", texID, ImVec2{ iconSize, iconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
			ImGui::OpenPopup("ViewportOverlaySettings");

		DrawOverlaySettingsPopup();

		ImGui::PopStyleColor(2);
		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
	}

	void EditorViewportPanel::DrawOverlaySettingsPopup()
	{
		if (!ImGui::BeginPopup("ViewportOverlaySettings"))
			return;

		auto& settings = m_context->viewportSettings;

		ImGui::Checkbox("Enable Overlay", &settings.showOverlay);
		ImGui::Separator();

		if (!settings.showOverlay)
			ImGui::BeginDisabled();

		ImGui::TextUnformatted("Display");
		ImGui::Checkbox("Grid", &settings.showGrid);
		ImGui::Checkbox("Origin", &settings.showOrigin);
		ImGui::Checkbox("Camera Bounds", &settings.showCameraBounds);
		ImGui::Checkbox("Camera Safe Area", &settings.showCameraSafeArea);
		ImGui::Checkbox("Colliders", &settings.showColliders);
		ImGui::Checkbox("Selection Bounds", &settings.showSelectionBounds);
		ImGui::Checkbox("Pivot", &settings.showPivot);
		ImGui::Checkbox("Relationship Lines", &settings.showRelationshipLines);

		ImGui::Separator();
		ImGui::TextUnformatted("Grid");

		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("Major Step", &settings.gridMajorStep, 0.05f, 0.05f, 100.0f, "%.2f"))
			settings.gridMajorStep = std::max(settings.gridMajorStep, 0.05f);

		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("Minor Step", &settings.gridMinorStep, 0.01f, 0.01f, 100.0f, "%.2f"))
			settings.gridMinorStep = std::max(settings.gridMinorStep, 0.01f);

		ImGui::Separator();
		ImGui::TextUnformatted("Snap");

		ImGui::Checkbox("Translate Snap", &settings.enableTranslationSnap);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("Translate Step", &settings.translationSnapValue, 0.05f, 0.01f, 100.0f, "%.2f"))
			settings.translationSnapValue = std::max(settings.translationSnapValue, 0.01f);

		ImGui::Checkbox("Rotate Snap", &settings.enableRotationSnap);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("Rotate Step", &settings.rotationSnapValue, 1.0f, 1.0f, 360.0f, "%.1f"))
			settings.rotationSnapValue = std::max(settings.rotationSnapValue, 1.0f);

		ImGui::Checkbox("Scale Snap", &settings.enableScaleSnap);
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::DragFloat("Scale Step", &settings.scaleSnapValue, 0.01f, 0.01f, 10.0f, "%.2f"))
			settings.scaleSnapValue = std::max(settings.scaleSnapValue, 0.01f);

		if (!settings.showOverlay)
			ImGui::EndDisabled();

		ImGui::EndPopup();
	}

	void EditorViewportPanel::OnEvent(Event& e)
	{
		// 相机滚轮事件
		if (m_context->viewport.hovered)
			m_editorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event)
		{
			return OnMouseButtonPressed(event);
		});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			return OnKeyPressed(event);
		});
	}

	bool EditorViewportPanel::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			bool altPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
			if (m_context->viewport.hovered && !altPressed)
			{
				bool ctrlPressed = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
				bool shiftPressed = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

				Entity hovered = m_context->viewport.hoveredEntity;

				if (hovered)
				{
					UUID hoveredUUID = hovered.GetUUID();

					if (ctrlPressed)
					{
						// Ctrl 如果已经选择，则移除
						m_context->selection.ToggleEntity(hoveredUUID);
					}
					else if (shiftPressed)
					{
						// Shift+Click：追加选择
						m_context->selection.AddEntity(hoveredUUID);
					}
					else
					{
						// 普通点击：单选
						m_context->selection.SetSelectedEntity(hoveredUUID);
					}
				}
				else if (!ImGuizmo::IsOver())
				{
					// 点击空白处：启动潜在框选
					auto& viewportState = m_context->viewport;
					auto [mx, my] = ImGui::GetMousePos();
					m_boxSelectStart = { mx - viewportState.bounds[0].x, my - viewportState.bounds[0].y };
					m_boxSelectEnd = m_boxSelectStart;
					m_boxSelectPending = true;
					m_boxSelectAdditive = shiftPressed || ctrlPressed;

					if (m_boxSelectAdditive)
						m_boxSelectSnapshot = m_context->selection.selectedEntities;
					else
						m_boxSelectSnapshot.clear();
				}
			}
		}

		return false;
	}


	bool EditorViewportPanel::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.IsRepeat())
			return false;

		bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		switch (e.GetKeyCode())
		{
		case Key::Q:
			if (m_context->runtime.IsEditing() && m_context->viewport.hovered && !ctrl && !shift)
				m_gizmoType = -1;
			break;
		case Key::W:
			if (m_context->runtime.IsEditing() && m_context->viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			if (m_context->runtime.IsEditing() && m_context->viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			if (m_context->runtime.IsEditing() && m_context->viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		case Key::F:
			if (m_context->runtime.IsEditing() && m_context->viewport.hovered && !ctrl && !shift)
			{
				UUID selectedUUID = m_context->selection.GetPrimarySelectedEntityUUID();
				if (selectedUUID != 0)
				{
					Entity entity = m_context->activeScene->FindEntityByUUID(selectedUUID);
					if (entity)
					{
						auto worldTransform = m_context->activeScene->GetWorldSpaceTransform(entity);
						m_editorCamera.SetPosition({ worldTransform.Translation.x, worldTransform.Translation.y, 0.0f });
					}
				}
			}
			break;
		}

		return false;
	}

	void EditorViewportPanel::UpdateBoxSelection()
	{
		if (!m_boxSelectPending && !m_isBoxSelecting)
			return;

		auto& viewportState = m_context->viewport;
		auto [mx, my] = ImGui::GetMousePos();
		float currentX = mx - viewportState.bounds[0].x;
		float currentY = my - viewportState.bounds[0].y;

		bool mouseHeld = Input::IsMouseButtonPressed(Mouse::ButtonLeft);

		// 潜在框选阶段：等待拖拽阈值
		if (m_boxSelectPending)
		{
			if (!mouseHeld)
			{
				// 释放但未拖拽 → 普通空白点击
				if (!m_boxSelectAdditive)
					m_context->selection.ClearEntitySelection();
				m_boxSelectPending = false;
				return;
			}

			// 检查拖拽距离阈值（3 像素）
			float dx = currentX - m_boxSelectStart.x;
			float dy = currentY - m_boxSelectStart.y;
			if (dx * dx + dy * dy > 9.0f)
			{
				m_isBoxSelecting = true;
				m_boxSelectPending = false;

				// 非追加模式：清空当前选择
				if (!m_boxSelectAdditive)
					m_context->selection.ClearEntitySelection();
			}
			else
			{
				return;
			}
		}

		// 框选进行中
		if (m_isBoxSelecting)
		{
			m_boxSelectEnd = { currentX, currentY };

			if (!mouseHeld)
			{
				// 鼠标释放 → 结束框选
				m_isBoxSelecting = false;
				return;
			}

			// 视口坐标→世界坐标（Y 翻转）
			float vpMinX = glm::min(m_boxSelectStart.x, m_boxSelectEnd.x);
			float vpMaxX = glm::max(m_boxSelectStart.x, m_boxSelectEnd.x);
			float vpMinY = glm::min(m_boxSelectStart.y, m_boxSelectEnd.y);
			float vpMaxY = glm::max(m_boxSelectStart.y, m_boxSelectEnd.y);

			glm::vec2 worldBL = m_editorCamera.ScreenToWorld(vpMinX, vpMaxY);
			glm::vec2 worldTR = m_editorCamera.ScreenToWorld(vpMaxX, vpMinY);

			// 从快照恢复（追加模式）或清空
			m_context->selection.selectedEntities = m_boxSelectSnapshot;

			// 遍历实体，将中心点在 AABB 内的实体加入选择
			auto view = m_context->activeScene->GetAllEntitiesWith<TransformComponent>();
			for (auto entityHandle : view)
			{
				Entity entity(entityHandle, m_context->activeScene.get());
				TransformComponent worldTC = m_context->activeScene->GetWorldSpaceTransform(entity);

				float ex = worldTC.Translation.x;
				float ey = worldTC.Translation.y;

				if (ex >= worldBL.x && ex <= worldTR.x && ey >= worldBL.y && ey <= worldTR.y)
					m_context->selection.AddEntity(entity.GetUUID());
			}
		}
	}

	void EditorViewportPanel::DrawBoxSelectionOverlay()
	{
		auto& viewportState = m_context->viewport;

		// 视口坐标转屏幕坐标
		ImVec2 p1 = {
			viewportState.bounds[0].x + m_boxSelectStart.x,
			viewportState.bounds[0].y + m_boxSelectStart.y
		};
		ImVec2 p2 = {
			viewportState.bounds[0].x + m_boxSelectEnd.x,
			viewportState.bounds[0].y + m_boxSelectEnd.y
		};

		auto* drawList = ImGui::GetWindowDrawList();

		// 半透明蓝色填充
		drawList->AddRectFilled(p1, p2, IM_COL32(80, 140, 230, 40));
		// 蓝色边框
		drawList->AddRect(p1, p2, IM_COL32(80, 140, 230, 200), 0.0f, 0, 1.5f);
	}

}
