#include "EditorLayer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

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

		// 初始化视口面板
		m_viewportPanel.SetContext(&m_editorContext);
		m_viewportPanel.SetRenderPipeline(&m_renderPipeline);
		m_viewportPanel.SetSceneController(&m_sceneController);
		m_viewportPanel.SetDirtyTracker(&m_dirtyTracker);
		m_viewportPanel.SetCommandHistory(&m_commandHistory);
		m_viewportPanel.Init();

		// 初始化编辑器服务
		m_sceneController.SetContext(&m_editorContext);
		m_sceneController.SetDirtyTracker(&m_dirtyTracker);
		m_sceneController.SetOnSceneChanged([this]() { OnSceneChanged(); });

		// Dirty Tracker
		m_dirtyTracker.SetContext(&m_editorContext);
		m_dirtyTracker.SetIsSafeToAutoSave([this]() { return !m_viewportPanel.IsGizmoInUse(); });
		m_dirtyTracker.SetAutoSaveCallback([this]() { m_sceneController.SaveScene(); });

		// CommandHistory → DirtyTracker 联动
		m_commandHistory.SetOnCommandExecuted([this]() { m_dirtyTracker.MarkSceneDirty(); });

		// 面板共享选择上下文
		m_sceneHierarchyPanel.SetSelectionContext(&m_editorContext.selection);
		m_sceneHierarchyPanel.SetDirtyTracker(&m_dirtyTracker);
		m_sceneHierarchyPanel.SetCommandHistory(&m_commandHistory);
		
		m_propertiesPanel.SetSelectionContext(&m_editorContext.selection);
		m_propertiesPanel.SetDirtyTracker(&m_dirtyTracker);
		m_propertiesPanel.SetCommandHistory(&m_commandHistory);

		// 创建默认场景
		m_sceneController.NewScene();

		// 编辑器不阻塞事件
		Application::Get().GetImGuiLayer()->BlockEvents(false);
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnSceneChanged()
	{
		// 场景切换后更新面板上下文
		m_sceneHierarchyPanel.SetContext(m_editorContext.activeScene);
		m_propertiesPanel.SetContext(m_editorContext.activeScene);
		m_viewportPanel.OnSceneChanged();

		// 场景切换后清空命令历史（旧命令引用旧场景对象）
		m_commandHistory.Clear();
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_viewportPanel.OnUpdate(ts);
		m_dirtyTracker.OnUpdate(ts);
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
		m_viewportPanel.OnImGuiRender();

		// State 面板
		OnImGuiDrawStateRender();

		// Scene Hierarchy
		m_sceneHierarchyPanel.OnImGuiRender();

		// Properties
		m_propertiesPanel.OnImGuiRender();

		// Content Browser
		m_contentBrowserPanel.OnImGuiRender();

		// SceneController 模态框
		m_sceneController.OnImGuiRender();

		// 窗口关闭确认对话框
		HandleWindowClose();

		ImGui::End(); // DockSpace
	}

	void EditorLayer::HandleWindowClose()
	{
		if (m_showCloseConfirmDialog)
		{
			ImGui::OpenPopup("Unsaved Changes##CloseEditor");
			m_showCloseConfirmDialog = false;

			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}

		if (!ImGui::BeginPopupModal("Unsaved Changes##CloseEditor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		ImGui::Text("The current scene has unsaved changes.");
		ImGui::Text("Do you want to save before closing?");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float buttonWidth = 100.0f;
		float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

		if (ImGui::Button("Save", ImVec2(buttonWidth, 0)))
		{
			m_sceneController.SaveScene();
			m_pendingClose = true;
			Application::Get().GetWindow().Close();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(buttonWidth, 0)))
		{
			m_pendingClose = true;
			Application::Get().GetWindow().Close();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
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

		// 居中标题文字（包含未保存标记）
		{
			std::string title = "YuiStudio";
			if (auto activeProject = Project::GetActive())
			{
				const std::string& projectName = activeProject->GetConfig().Name;
				if (!projectName.empty())
					title += " - " + projectName;
			}

			if (m_dirtyTracker.IsSceneDirty())
				title += " *";

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
		// 视口面板处理相机、选择、Gizmo 快捷键
		m_viewportPanel.OnEvent(e);

		EventDispatcher dispatcher(e);

		// 标题栏命中测试
		dispatcher.Dispatch<WindowTitleBarHitTestEvent>([this](WindowTitleBarHitTestEvent& event)
		{
			event.SetHit(m_titleBarHovered);
			return true;
		});

		// 窗口关闭拦截
		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& event)
		{
			if (m_pendingClose)
				return false;

			if (m_dirtyTracker.IsSceneDirty())
			{
				m_showCloseConfirmDialog = true;
				Application::Get().GetWindow().CancelClose();
				return true;
			}

			return false;
		});

		// 编辑器快捷键
		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& event)
		{
			return OnKeyPressed(event);
		});
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& e)
	{
		if (e.IsRepeat())
			return false;

		// 文字输入控件激活时不处理快捷键
		if (ImGui::GetIO().WantTextInput)
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
		case Key::Z:
			YUICY_CORE_INFO("ctrl: {}, shift: {}", ctrl, shift);
			if (ctrl && !shift) m_commandHistory.Undo();
			break;
		case Key::Y:
			if (ctrl) m_commandHistory.Redo();
			break;
		}

		return false;
	}

}
