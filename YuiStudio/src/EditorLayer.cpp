#include "EditorLayer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <fstream>

// Win32 文件对话框
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Yuicy {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnAttach()
	{
		// 创建 Framebuffer
		FramebufferSpecification fbSpec;
		fbSpec.attachments = { FramebufferTextureFormat::RGBA8, FramebufferTextureFormat::RED_INTEGER, FramebufferTextureFormat::Depth };
		fbSpec.width = 1280;
		fbSpec.height = 720;
		m_framebuffer = Framebuffer::Create(fbSpec);

		// 创建默认场景
		m_editorScene = CreateRef<Scene>();
		m_activeScene = m_editorScene;
		m_editorScene->SetName("UntitledScene");

		// 编辑器相机
		m_editorCamera = EditorCamera(1280.0f / 720.0f, 5.0f);

		// 测试实体
		auto testEntity = m_editorScene->CreateEntity("TestSprite");
		testEntity.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.2f, 0.6f, 0.9f, 1.0f });

		// 初始化面板
		m_sceneHierarchyPanel.SetContext(m_activeScene);
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		// Viewport resize
		auto spec = m_framebuffer->GetSpecification();
		if (m_viewportSize.x > 0.0f && m_viewportSize.y > 0.0f
			&& (spec.width != (uint32_t)m_viewportSize.x || spec.height != (uint32_t)m_viewportSize.y))
		{
			m_framebuffer->Resize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
			m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
			m_editorCamera.SetViewportSize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		}

		// 渲染到 Framebuffer
		Renderer2D::ResetStats();
		m_framebuffer->Bind();

		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();

		// 清除实体 ID 附件为 -1
		m_framebuffer->ClearAttachment(1, -1);

		switch (m_sceneState)
		{
		case SceneState::Edit:
			if (m_viewportFocused)
				m_editorCamera.OnUpdate(ts);
			m_activeScene->OnUpdateEditor(ts, m_editorCamera);
			break;
		case SceneState::Play:
			m_activeScene->OnUpdateRuntime(ts);
			break;
		}

		m_framebuffer->Unbind();
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

		// Scene Hierarchy + Properties
		m_sceneHierarchyPanel.OnImGuiRender();

		ImGui::End(); // DockSpace
	}

	void EditorLayer::OnImGuiViewportRender()
	{
		// 无边框
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		// 检查当前窗口的焦点/悬停状态
		m_viewportFocused = ImGui::IsWindowFocused();
		m_viewportHovered = ImGui::IsWindowHovered();

		// 当视口聚焦或悬停时，不阻塞鼠标/键盘事件
		Application::Get().GetImGuiLayer()->BlockEvents(!m_viewportFocused && !m_viewportHovered);

		// GetContentRegionAvail() → 当前窗口内"可用区域"的尺寸 更新绘制时的视口大小
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		// 获取 Framebuffer 颜色附件的 纹理 ID
		uint64_t textureID = m_framebuffer->GetColorAttachmentRendererID();

		// ImGui::Image() → 绘制渲染结果
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

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
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
		ImGui::Separator();
		ImGui::Text("Scene: %s", m_activeScene->GetName().c_str());

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
				if (ImGui::MenuItem("New Scene", "Ctrl+N"))
					NewScene();

				if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
					OpenScene();

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
					SaveScene();

				if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
					SaveSceneAs();

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
			const char* title = "YuiStudio";
			ImVec2 textSize = ImGui::CalcTextSize(title);
			float textX = titlebarMin.x + (windowWidth - textSize.x) * 0.5f;
			float textY = titlebarMin.y + (titlebarHeight - textSize.y) * 0.5f;
			drawList->AddText(ImVec2(textX, textY), IM_COL32(140, 140, 140, 255), title);
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
		if (m_viewportHovered)
			m_editorCamera.OnEvent(e);

		EventDispatcher dispatcher(e);

		// 标题栏命中测试
		dispatcher.Dispatch<WindowTitleBarHitTestEvent>([this](WindowTitleBarHitTestEvent& event)
		{
			event.SetHit(m_titleBarHovered);
			return true;
		});

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) -> bool
		{
			if (e.IsRepeat())
				return false;

			bool ctrl = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
			bool shift = Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift);

			switch (e.GetKeyCode())
			{
			case Key::N:
				if (ctrl) NewScene();
				break;
			case Key::O:
				if (ctrl) OpenScene();
				break;
			case Key::S:
				if (ctrl && shift) SaveSceneAs();
				else if (ctrl) SaveScene();
				break;
			}
			return false;
		});
	}

	// 场景操作
	void EditorLayer::NewScene()
	{
		m_editorScene = CreateRef<Scene>();
		m_editorScene->SetName("UntitledScene");
		m_editorScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_activeScene = m_editorScene;
		m_currentScenePath = std::filesystem::path{};
		m_sceneHierarchyPanel.SetContext(m_activeScene);
	}

	void EditorLayer::OpenScene()
	{
		std::string filepath = OpenFileDialog(Yuicy::SceneSerializer::GetSceneSerializerFileFilter());
		if (!filepath.empty())
		{
			m_editorScene = CreateRef<Scene>();
			m_editorScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

			SceneSerializer serializer(m_editorScene);
			if (serializer.Deserialize(filepath))
			{
				m_activeScene = m_editorScene;
				m_currentScenePath = filepath;
				m_sceneHierarchyPanel.SetContext(m_activeScene);
				YUICY_CORE_INFO("Opened scene: {}", filepath);
			}
		}
	}

	void EditorLayer::SaveScene()
	{
		if (!m_currentScenePath.empty())
		{
			SceneSerializer serializer(m_activeScene);
			serializer.Serialize(m_currentScenePath);
		}
		else
		{
			SaveSceneAs();
		}
	}

	void EditorLayer::SaveSceneAs()
	{
		std::string filepath = SaveFileDialog(Yuicy::SceneSerializer::GetSceneSerializerFileFilter());
		if (!filepath.empty())
		{
			// 确保有 .yui 后缀
			if (filepath.find(".yui") == std::string::npos)
				filepath += ".yui";

			SceneSerializer serializer(m_activeScene);
			serializer.Serialize(filepath);
			m_currentScenePath = filepath;
		}
	}

	// Win32 文件对话框
	std::string EditorLayer::OpenFileDialog(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}

	std::string EditorLayer::SaveFileDialog(const char* filter)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
		ofn.lpstrDefExt = "yui";

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}

}
