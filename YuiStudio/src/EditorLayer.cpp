#include "EditorLayer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <string>

namespace Yuicy {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		// 创建渲染管线
		FramebufferSpecification fbSpec;
		fbSpec.attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.width = 1280;
		fbSpec.height = 720;
		m_renderPipeline.Init(fbSpec);
		m_renderPipeline.SetContext(&m_editorContext);
		m_renderPipeline.SetOverlayRenderer(&m_overlayRenderer);

		// 初始化 Overlay 渲染器
		m_overlayRenderer.SetContext(&m_editorContext);

		// 编辑器相机
		m_editorCamera = EditorCamera(1280.0f / 720.0f, 5.0f);

		// 初始化编辑器服务
		m_sceneController.SetContext(&m_editorContext);
		m_sceneController.SetOnSceneChanged([this]() { OnSceneChanged(); });
		m_dirtyTracker.SetContext(&m_editorContext);

		// 面板共享选择上下文
		m_sceneHierarchyPanel.SetSelectionContext(&m_editorContext.selection);
		m_propertiesPanel.SetSelectionContext(&m_editorContext.selection);

		// 创建默认场景
		m_sceneController.NewScene();

		// 编辑器图标
		m_playIcon = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Play.png", { 50, 200, 50, 255 });
		m_stopIcon = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Stop.png", { 200, 50, 50, 255 });
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnSceneChanged()
	{
		// 场景切换后更新面板上下文
		m_sceneHierarchyPanel.SetContext(m_editorContext.activeScene);
		m_propertiesPanel.SetContext(m_editorContext.activeScene);
		m_gizmoType = -1;
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		auto& viewportState = m_editorContext.viewport;

		// Viewport resize
		auto fbSpec = m_renderPipeline.GetFramebuffer()->GetSpecification();
		if (viewportState.size.x > 0.0f && viewportState.size.y > 0.0f
			&& (fbSpec.width != (uint32_t)viewportState.size.x || fbSpec.height != (uint32_t)viewportState.size.y))
		{
			uint32_t width = (uint32_t)viewportState.size.x;
			uint32_t height = (uint32_t)viewportState.size.y;
			// TODO: 考虑将视口大小抽象到上下文中
			m_renderPipeline.OnViewportResize(width, height);
			m_editorContext.activeScene->OnViewportResize(width, height);
			m_editorCamera.SetViewportSize(width, height);
		}

		// 编辑器相机更新
		if (m_editorContext.runtime.mode != SceneMode::Play && viewportState.focused)
			m_editorCamera.OnUpdate(ts);

		// 执行渲染管线：Scene Pass + Overlay Pass
		m_renderPipeline.Execute(ts, m_editorCamera);

		// 鼠标拾取
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
			int pixelData = m_renderPipeline.ReadEntityIDAtPixel(mouseX, mouseY);
			viewportState.hoveredEntity = pixelData == -1 ? Entity{} : Entity((entt::entity)pixelData, m_editorContext.activeScene.get());
		}
		else
		{
			viewportState.hoveredEntity = {};
		}
	}

	void EditorLayer::OnImGuiRender()
	{
		// Dockspace 设置
		static bool dockspaceOpen = true;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking;

		// GetMainViewport() 返回操作系统窗口的可用区域信息
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);	// 无边框
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		// 设置窗口背景：不能移动、不能缩放、无标题栏
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse
			        |  ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoMove;
		// 不抢占焦点，不参与键盘导航
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f)); // 窗口内边距 = 0

		// 初始化 "DockSpace" 背景
		ImGui::Begin("DockSpace", &dockspaceOpen, windowFlags);
		ImGui::PopStyleVar(3);

		// 标题栏
		float titlebarHeight = UIDrawTitlebar();

		// DockSpace 从标题栏下方开始
		ImGui::SetCursorPosY(titlebarHeight);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspaceId = ImGui::GetID("YuiStudioDockspace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		style.WindowMinSize.x = minWinSizeX;

		// Viewport 面板
		OnImGuiViewportRender();
		// State 面板
		OnImGuiDrawStateRender();
		// Play/Stop 工具栏
		OnImGuiToolbarRender();

		// Scene Hierarchy
		m_sceneHierarchyPanel.OnImGuiRender();

		// Properties
		m_propertiesPanel.OnImGuiRender();

		// Content Browser
		m_contentBrowserPanel.OnImGuiRender();

		ImGui::End(); // DockSpace
	}

	void EditorLayer::OnImGuiViewportRender()
	{
		auto& viewportState = m_editorContext.viewport;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		viewportState.focused = ImGui::IsWindowFocused();
		viewportState.hovered = ImGui::IsWindowHovered();

		// 当视口聚焦或悬停时，不阻塞鼠标/键盘事件
		Application::Get().GetImGuiLayer()->BlockEvents(!viewportState.focused && !viewportState.hovered);

		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		viewportState.size = { viewportPanelSize.x, viewportPanelSize.y };

		// 计算 Viewport 的屏幕坐标
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportOffset = ImGui::GetWindowPos();
		viewportState.bounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		viewportState.bounds[1] = viewportState.bounds[0] + viewportState.size;

		uint64_t textureID = m_renderPipeline.GetColorAttachmentRendererID();
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ viewportState.size.x, viewportState.size.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		// 接收从 ContentBrowser 拖拽过来的场景文件
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const auto* pathData = (const std::filesystem::path::value_type*)payload->Data;
				std::filesystem::path filepath(pathData);
				if (filepath.extension() == SceneSerializer::GetSceneSerializerDefaultExtension())
					m_sceneController.OpenScene(filepath);
			}

			ImGui::EndDragDropTarget();
		}

		// Gizmo 绘制
		OnImGuiDrawGizmos();

		ImGui::End(); // Viewport
		ImGui::PopStyleVar();
	}

	void EditorLayer::OnImGuiDrawStateRender()
	{
		ImGui::Begin("Stats");

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Lines: %d", stats.LineCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
		ImGui::Separator();
		ImGui::Text("Scene: %s", m_editorContext.activeScene->GetName().c_str());
		std::string hoveredName = "None";
		if (m_editorContext.viewport.hoveredEntity)
			hoveredName = m_editorContext.viewport.hoveredEntity.GetComponent<TagComponent>().Tag;
		ImGui::Text("Hovered Entity: %s", hoveredName.c_str());

		ImGui::End();
	}

	void EditorLayer::OnImGuiToolbarRender()
	{
		auto& viewportState = m_editorContext.viewport;

		if (viewportState.size.x <= 0.0f || viewportState.size.y <= 0.0f)
			return;

		const bool isPlaying = m_editorContext.runtime.mode == SceneMode::Play;
		Ref<Texture2D> icon = isPlaying ? m_stopIcon : m_playIcon;

		const float edgeOffset = 8.0f;
		const float iconSize = 24.0f;
		const float windowWidth = iconSize + edgeOffset * 2.0f;
		const float windowHeight = iconSize + edgeOffset;

		float toolbarX = (viewportState.bounds[0].x + viewportState.bounds[1].x) * 0.5f;
		ImGui::SetNextWindowPos(ImVec2(toolbarX - windowWidth * 0.5f, viewportState.bounds[0].y + edgeOffset));
		ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
		ImGui::SetNextWindowBgAlpha(0.75f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(edgeOffset, edgeOffset * 0.5f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

		ImGui::Begin("##toolbar", nullptr,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		const ImVec4 tintNormal = ImVec4(1, 1, 1, 0.8f);
		const ImVec4 tintHovered = ImVec4(1, 1, 1, 1.0f);

		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 0.7f));

		ImTextureID texID = reinterpret_cast<ImTextureID>((uintptr_t)icon->GetRendererID());
		if (ImGui::ImageButton("##PlayStop", texID, ImVec2{ iconSize, iconSize }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 }))
		{
			if (isPlaying)
				m_sceneController.OnSceneStop();
			else
				m_sceneController.OnScenePlay();
		}

		ImGui::PopStyleColor(2);
		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(3);
	}

	// 标题栏
	float EditorLayer::UIDrawTitlebar()
	{
		const float titlebarHeight = 40.0f;
		const float buttonWidth = 46.0f;
		const float totalButtonsWidth = buttonWidth * 3;
		const ImVec2 windowPadding = ImGui::GetCurrentWindow()->WindowPadding;

		const ImVec2 titlebarMin = ImGui::GetCursorScreenPos();
		const float windowWidth = ImGui::GetWindowWidth();
		const ImVec2 titlebarMax = {
			titlebarMin.x + windowWidth,
			titlebarMin.y + titlebarHeight };

		// 绘制标题栏背景
		auto* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(titlebarMin, titlebarMax, IM_COL32(25, 25, 25, 255));

		// 菜单栏
		float dragZoneWidth = windowWidth - totalButtonsWidth;

		ImGui::SetCursorPos(ImVec2(windowPadding.x + 6.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
		ImGui::BeginChild("##menuBarChild", ImVec2(dragZoneWidth * 0.5f, titlebarHeight - 4.0f),
			false, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground
			     | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNavFocus);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Project..."))
					m_sceneController.NewProject();

				if (ImGui::MenuItem("Open Project..."))
					m_sceneController.OpenProjectDialog();

				if (ImGui::MenuItem("Save Project"))
					m_sceneController.SaveProject();

				ImGui::Separator();

				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					m_sceneController.NewScene();

				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					m_sceneController.OpenSceneDialog();

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					m_sceneController.SaveScene();

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					m_sceneController.SaveSceneAs();

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
					Application::Get().GetWindow().Close();

				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
		bool menuHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
		ImGui::EndChild();
		ImGui::PopStyleVar();

		// 标题栏悬停判断
		{
			ImVec2 mousePos = ImGui::GetMousePos();
			bool inTitlebar = mousePos.x >= titlebarMin.x && mousePos.x < titlebarMin.x + dragZoneWidth
				           && mousePos.y >= titlebarMin.y && mousePos.y < titlebarMax.y;
			m_titleBarHovered = inTitlebar && !menuHovered;
		}

		// 居中标题文字
		{
			std::string title = "YuiStudio";
			if (auto activeProject = Project::GetActive())
			{
				const std::string& projectName = activeProject->GetConfig().Name;
				if (!projectName.empty())
					title += " - " + projectName;
			}

			ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
			float textX = titlebarMin.x + (windowWidth - textSize.x) * 0.5f;
			float textY = titlebarMin.y + (titlebarHeight - textSize.y) * 0.5f;
			drawList->AddText(ImVec2(textX, textY), IM_COL32(140, 140, 140, 255), title.c_str());
		}

		// 窗口控制按钮
		auto& appWindow = Application::Get().GetWindow();
		bool isMaximized = appWindow.IsMaximized();
		float buttonsStartX = windowWidth - totalButtonsWidth;

		auto drawWindowButton = [&](const char* id, float posX, auto onClickFn, auto drawIconFn, ImU32 hoverColor)
		{
			ImGui::SetCursorPos(ImVec2(posX, 0));
			ImGui::PushID(id);

			if (ImGui::InvisibleButton(id, ImVec2(buttonWidth, titlebarHeight)))
				onClickFn();

			ImVec2 btnMin = ImGui::GetItemRectMin();
			ImVec2 btnMax = ImGui::GetItemRectMax();
			bool hovered = ImGui::IsItemHovered();

			if (hovered)
				drawList->AddRectFilled(btnMin, btnMax, hoverColor);

			float cx = (btnMin.x + btnMax.x) * 0.5f;
			float cy = (btnMin.y + btnMax.y) * 0.5f;
			ImU32 iconCol = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255);
			drawIconFn(cx, cy, iconCol);

			ImGui::PopID();
		};

		// 最小化
		drawWindowButton("##minimize", buttonsStartX,
			[&]() { appWindow.Minimize(); },
			[&](float cx, float cy, ImU32 col) {
				drawList->AddLine(ImVec2(cx - 6, cy), ImVec2(cx + 6, cy), col, 1.5f);
			},
			IM_COL32(60, 60, 60, 255));

		// 最大化/还原
		drawWindowButton("##maximize", buttonsStartX + buttonWidth,
			[&]() { if (isMaximized) appWindow.Restore(); else appWindow.Maximize(); },
			[&](float cx, float cy, ImU32 col) {
				if (isMaximized) {
					drawList->AddRect(ImVec2(cx - 4, cy - 2), ImVec2(cx + 4, cy + 6), col, 0.0f, 0, 1.5f);
					drawList->AddRect(ImVec2(cx - 2, cy - 5), ImVec2(cx + 6, cy + 3), col, 0.0f, 0, 1.5f);
				} else {
					drawList->AddRect(ImVec2(cx - 5, cy - 5), ImVec2(cx + 5, cy + 5), col, 0.0f, 0, 1.5f);
				}
			},
			IM_COL32(60, 60, 60, 255));

		// 关闭
		drawWindowButton("##close", buttonsStartX + buttonWidth * 2,
			[&]() { appWindow.Close(); },
			[&](float cx, float cy, ImU32 col) {
				drawList->AddLine(ImVec2(cx - 5, cy - 5), ImVec2(cx + 5, cy + 5), col, 1.5f);
				drawList->AddLine(ImVec2(cx + 5, cy - 5), ImVec2(cx - 5, cy + 5), col, 1.5f);
			},
			IM_COL32(210, 50, 50, 255));

		return titlebarHeight;
	}

	void EditorLayer::OnEvent(Event& e)
	{
		if (m_editorContext.viewport.hovered)
			m_editorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);

		// 标题栏命中测试
		dispatcher.Dispatch<WindowTitleBarHitTestEvent>([this](WindowTitleBarHitTestEvent& event)
		{
			event.SetHit(m_titleBarHovered);
			return true;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& event)
		{
			return OnMouseButtonPressed(event);
		});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			return OnKeyPressed(event);
		});
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e)
	{
		if (e.GetMouseButton() == Mouse::ButtonLeft)
		{
			bool altPressed = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
			if (m_editorContext.viewport.hovered && !altPressed)
			{
				// 写入共享选择上下文
				Entity hovered = m_editorContext.viewport.hoveredEntity;
				if (hovered)
					m_editorContext.selection.SetSelectedEntity(hovered.GetUUID());
				else
					m_editorContext.selection.ClearEntitySelection();
			}
		}

		return false;
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.IsRepeat())
			return false;

		bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

		switch (e.GetKeyCode())
		{
		case Key::N:
			if (ctrl) m_sceneController.NewScene();
			break;
		case Key::O:
			if (ctrl) m_sceneController.OpenSceneDialog();
			break;
		case Key::S:
			if (ctrl && shift) m_sceneController.SaveSceneAs();
			else if (ctrl) m_sceneController.SaveScene();
			break;
		case Key::Q:
			if (m_editorContext.runtime.IsEditing() && m_editorContext.viewport.hovered && !ctrl && !shift)
				m_gizmoType = -1;
			break;
		case Key::W:
			if (m_editorContext.runtime.IsEditing() && m_editorContext.viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			if (m_editorContext.runtime.IsEditing() && m_editorContext.viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			if (m_editorContext.runtime.IsEditing() && m_editorContext.viewport.hovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}

		return false;
	}

	void EditorLayer::OnImGuiDrawGizmos()
	{
		if (!m_editorContext.runtime.IsEditing())
			return;

		// 从共享选择上下文解析选中实体
		UUID selectedUUID = m_editorContext.selection.GetPrimarySelectedEntityUUID();
		if (selectedUUID == 0 || m_gizmoType == -1)
			return;

		Entity selectedEntity = m_editorContext.activeScene->FindEntityByUUID(selectedUUID);
		if (!selectedEntity)
			return;

		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(
			m_editorContext.viewport.bounds[0].x, m_editorContext.viewport.bounds[0].y,
			m_editorContext.viewport.size.x, m_editorContext.viewport.size.y);

		const glm::mat4& cameraProjection = m_editorCamera.GetProjection();
		glm::mat4 cameraView = m_editorCamera.GetViewMatrix();
		glm::mat4 worldTransform = m_editorContext.activeScene->GetWorldSpaceTransformMatrix(selectedEntity);

		// 吸附设置来自 EditorViewportSettings
		auto& snapSettings = m_editorContext.viewportSettings;
		bool snap = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		float snapValue = 0.5f;
		if (m_gizmoType == ImGuizmo::OPERATION::ROTATE)
			snapValue = 45.0f;
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

		if (ImGuizmo::IsUsing())
		{
			glm::mat4 localTransform = worldTransform;
			Entity parent = selectedEntity.GetParent();
			if (parent)
			{
				glm::mat4 parentWorldTransform = m_editorContext.activeScene->GetWorldSpaceTransformMatrix(parent);
				localTransform = glm::inverse(parentWorldTransform) * worldTransform;
			}

			selectedEntity.GetComponent<TransformComponent>().SetTransform(localTransform);
		}
	}
}
