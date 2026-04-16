#include "EditorLayer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include <glm/gtc/type_ptr.hpp>

#include <filesystem>
#include <string>

// Win32 文件对话框
#include <commdlg.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace Yuicy {

	static bool SceneHasPrimaryCamera(const Ref<Scene>& scene)
	{
		if (!scene)
			return false;

		auto view = scene->GetAllEntitiesWith<CameraComponent>();
		for (auto entity : view)
		{
			if (view.get<CameraComponent>(entity).Primary)
				return true;
		}

		return false;
	}

	static void CreateDefaultPrimaryCamera(const Ref<Scene>& scene)
	{
		if (!scene || SceneHasPrimaryCamera(scene))
			return;

		auto cameraEntity = scene->CreateEntity("Camera");
		cameraEntity.AddComponent<CameraComponent>();
	}

	static void CreateDefaultSceneContent(const Ref<Scene>& scene)
	{
		if (!scene)
			return;

		scene->SetName("UntitledScene");
		CreateDefaultPrimaryCamera(scene);

		auto testEntity = scene->CreateEntity("TestSprite");
		testEntity.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.2f, 0.6f, 0.9f, 1.0f });
	}

	static std::filesystem::path GetDefaultProjectScenePath(const Ref<Project>& project)
	{
		YUICY_CORE_ASSERT(project);
		return project->GetAssetDirectory() / "Scenes" / ("StartScene" + std::string(SceneSerializer::GetSceneSerializerDefaultExtension()));
	}

	// 获取 filepath 相对于 directory 的相对路径
	static bool TryGetPathRelativeToDirectory(const std::filesystem::path& filepath, const std::filesystem::path& directory, 
		std::filesystem::path& outRelativePath)
	{
		if (filepath.empty() || directory.empty())
			return false;

		std::filesystem::path relativePath = filepath.lexically_normal().lexically_relative(directory.lexically_normal());
		if (relativePath.empty())
			return false;

		if (auto it = relativePath.begin(); it != relativePath.end() && it->string() == "..")  // 判断当前路径在 directory 目录下
			return false;

		outRelativePath = relativePath;
		return true;
	}

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
		CreateDefaultSceneContent(m_editorScene);

		// 编辑器相机
		m_editorCamera = EditorCamera(1280.0f / 720.0f, 5.0f);

		// 初始化面板
		m_sceneHierarchyPanel.SetContext(m_activeScene);

		// 编辑器图标
		m_playIcon = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Play.png", { 50, 200, 50, 255 });
		m_stopIcon = EditorIconUtils::LoadIconTexture("assets/textures/Editor/Viewport/Stop.png", { 200, 50, 50, 255 });
	}

	void EditorLayer::OnDetach()
	{
	}

	void EditorLayer::OnScenePlay()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		if (!SceneHasPrimaryCamera(m_editorScene))
		{
			YUICY_CORE_WARN("Cannot enter Play mode: scene has no primary camera.");
			return;
		}

		m_sceneState = SceneState::Play;

		m_activeScene = Scene::Copy(m_editorScene);
		m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_activeScene->OnRuntimeStart();

		m_hoveredEntity = {};
		m_sceneHierarchyPanel.SetContext(m_activeScene);
	}

	void EditorLayer::OnSceneStop()
	{
		if (m_sceneState != SceneState::Play || !m_activeScene)
			return;

		m_activeScene->OnRuntimeStop();
		m_sceneState = SceneState::Edit;

		m_activeScene = m_editorScene;
		m_activeScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_hoveredEntity = {};
		m_sceneHierarchyPanel.SetContext(m_activeScene);
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

		// 鼠标拾取
		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_viewportBounds[0].x;
		my -= m_viewportBounds[0].y;

		glm::vec2 viewportBoundsSize = m_viewportBounds[1] - m_viewportBounds[0];
		my = viewportBoundsSize.y - my;

		int mouseX = (int)mx;
		int mouseY = (int)my;

		if (mouseX >= 0 && mouseY >= 0
			&& mouseX < (int)viewportBoundsSize.x && mouseY < (int)viewportBoundsSize.y)
		{
			int pixelData = m_framebuffer->ReadPixel(1, mouseX, mouseY);
			m_hoveredEntity = pixelData == -1 ? Entity{} : Entity((entt::entity)pixelData, m_activeScene.get());
		}
		else
		{
			m_hoveredEntity = {};
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
		// Play/Stop 工具栏
		OnImGuiToolbarRender();

		// Scene Hierarchy + Properties
		m_sceneHierarchyPanel.OnImGuiRender();

		// Content Browser
		m_contentBrowserPanel.OnImGuiRender();

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

		// 计算Viewport的屏幕坐标
		auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
		auto viewportOffset = ImGui::GetWindowPos();
		m_viewportBounds[0] = { viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y };
		m_viewportBounds[1] = m_viewportBounds[0] + m_viewportSize;

		// 获取 Framebuffer 颜色附件的 纹理 ID
		uint64_t textureID = m_framebuffer->GetColorAttachmentRendererID();

		// ImGui::Image() → 绘制渲染结果
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		// 接收从 ContentBrowser 拖拽过来的场景文件
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
			{
				const auto* pathData = (const std::filesystem::path::value_type*)payload->Data;
				std::filesystem::path filepath(pathData);
				if (filepath.extension() == SceneSerializer::GetSceneSerializerDefaultExtension())
					OpenScene(filepath);
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
		ImGui::Text("Scene: %s", m_activeScene->GetName().c_str());
		std::string hoveredName = "None";
		if (m_hoveredEntity)
			hoveredName = m_hoveredEntity.GetComponent<TagComponent>().Tag;
		ImGui::Text("Hovered Entity: %s", hoveredName.c_str());

		ImGui::End();
	}

	void EditorLayer::OnImGuiToolbarRender()
	{
		if (m_viewportSize.x <= 0.0f || m_viewportSize.y <= 0.0f)
			return;

		const bool isPlaying = m_sceneState == SceneState::Play;
		Ref<Texture2D> icon = isPlaying ? m_stopIcon : m_playIcon;

		const float edgeOffset = 8.0f;
		const float iconSize = 24.0f;
		const float windowWidth = iconSize + edgeOffset * 2.0f;
		const float windowHeight = iconSize + edgeOffset;

		float toolbarX = (m_viewportBounds[0].x + m_viewportBounds[1].x) * 0.5f;
		ImGui::SetNextWindowPos(ImVec2(toolbarX - windowWidth * 0.5f, m_viewportBounds[0].y + edgeOffset));
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
				OnSceneStop();
			else
				OnScenePlay();
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
					NewProject();

				if (ImGui::MenuItem("Open Project..."))
					OpenProject();

				if (ImGui::MenuItem("Save Project"))
					SaveProject();

				ImGui::Separator();

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
		if (m_viewportHovered)
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
			if (m_viewportHovered && !altPressed)
				m_sceneHierarchyPanel.SetSelectedEntity(m_hoveredEntity);
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
			if (ctrl) NewScene();
			break;
		case Key::O:
			if (ctrl) OpenScene();
			break;
		case Key::S:
			if (ctrl && shift) SaveSceneAs();
			else if (ctrl) SaveScene();
			break;
		case Key::Q:
			if (m_sceneState == SceneState::Edit && m_viewportHovered && !ctrl && !shift)
				m_gizmoType = -1;
			break;
		case Key::W:
			if (m_sceneState == SceneState::Edit && m_viewportHovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			if (m_sceneState == SceneState::Edit && m_viewportHovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			if (m_sceneState == SceneState::Edit && m_viewportHovered && !ctrl && !shift)
				m_gizmoType = ImGuizmo::OPERATION::SCALE;
			break;
		}

		return false;
	}

	// 场景操作
	void EditorLayer::NewScene()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		m_editorScene = CreateRef<Scene>();
		CreateDefaultSceneContent(m_editorScene);
		m_editorScene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);
		m_activeScene = m_editorScene;
		m_currentScenePath = std::filesystem::path{};
		m_hoveredEntity = {};
		m_gizmoType = -1;
		m_sceneHierarchyPanel.SetContext(m_activeScene);
	}

	void EditorLayer::OpenScene()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		std::string filepath = OpenFileDialog(Yuicy::SceneSerializer::GetSceneSerializerFileFilter());
		if (!filepath.empty() && OpenScene(filepath))
		{
			YUICY_CORE_INFO("Opened scene: {}", filepath);
		}
	}

	bool EditorLayer::OpenScene(const std::filesystem::path& filepath)
	{
		if (m_sceneState != SceneState::Edit)
			return false;

		Ref<Scene> scene = CreateRef<Scene>();
		scene->OnViewportResize((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y);

		SceneSerializer serializer(scene);
		if (!serializer.Deserialize(filepath))
			return false;

		m_editorScene = scene;
		m_activeScene = m_editorScene;
		m_currentScenePath = filepath.lexically_normal();
		m_hoveredEntity = {};
		m_gizmoType = -1;
		m_sceneHierarchyPanel.SetContext(m_activeScene);
		return true;
	}

	void EditorLayer::SaveScene()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		if (!m_currentScenePath.empty())
		{
			SceneSerializer serializer(m_editorScene);
			serializer.Serialize(m_currentScenePath);

			if (Project::GetActive())
				SaveProject();
		}
		else
		{
			SaveSceneAs();
		}
	}

	void EditorLayer::SaveSceneAs()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		std::string filepath = SaveFileDialog(
			Yuicy::SceneSerializer::GetSceneSerializerFileFilter(),
			Yuicy::SceneSerializer::GetSceneSerializerDefaultExtension());
		if (!filepath.empty())
		{
			std::filesystem::path scenePath = filepath;
			if (scenePath.extension() != Yuicy::SceneSerializer::GetSceneSerializerDefaultExtension())
				scenePath += Yuicy::SceneSerializer::GetSceneSerializerDefaultExtension();

			SceneSerializer serializer(m_editorScene);
			serializer.Serialize(scenePath);
			m_currentScenePath = scenePath;

			if (Project::GetActive())
				SaveProject();
		}
	}

	void EditorLayer::NewProject()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		std::string filepath = SaveFileDialog(
			ProjectSerializer::GetProjectSerializerFileFilter(),
			ProjectSerializer::GetProjectSerializerDefaultExtension());
		if (filepath.empty())
			return;

		std::filesystem::path projectPath = filepath;
		if (projectPath.extension() != ProjectSerializer::GetProjectSerializerDefaultExtension())
			projectPath += ProjectSerializer::GetProjectSerializerDefaultExtension();

		auto project = CreateRef<Project>();
		project->GetConfig().Name = projectPath.stem().string();
		project->GetConfig().ProjectDirectory = projectPath.parent_path().string();
		project->GetConfig().ProjectFileName = projectPath.filename().string();

		std::error_code ec;
		std::filesystem::create_directories(project->GetAssetDirectory(), ec);
		if (ec)
		{
			YUICY_CORE_ERROR("[Project] Failed to create asset directory '{}': {}", project->GetAssetDirectory().string(), ec.message());
			return;
		}

		ec.clear();
		std::filesystem::path scriptDirectory = std::filesystem::path(project->GetConfig().ProjectDirectory) / project->GetConfig().ScriptDirectory;
		std::filesystem::create_directories(scriptDirectory, ec);
		if (ec)
		{
			YUICY_CORE_ERROR("[Project] Failed to create script directory '{}': {}", scriptDirectory.string(), ec.message());
			return;
		}

		Project::SetActive(project);
		m_currentProjectPath = projectPath.lexically_normal();

		if (!m_editorScene)
			NewScene();

		SaveProject();
	}

	void EditorLayer::OpenProject()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		std::string filepath = OpenFileDialog(ProjectSerializer::GetProjectSerializerFileFilter());
		if (!filepath.empty())
			OpenProject(filepath);
	}

	void EditorLayer::OpenProject(const std::filesystem::path& filepath)
	{
		if (m_sceneState != SceneState::Edit)
			return;

		auto project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(filepath))
			return;

		Project::SetActive(project);
		m_currentProjectPath = filepath;

		bool sceneLoaded = false;
		if (!project->GetConfig().StartScene.empty())
		{
			std::filesystem::path scenePath = Project::GetActiveAssetDirectory() / project->GetConfig().StartScene;
			if (std::filesystem::exists(scenePath))
			{
				sceneLoaded = OpenScene(scenePath);
			}
			else
			{
				YUICY_CORE_WARN("[Project] Start scene not found: {}", scenePath.string());
			}
		}

		if (!sceneLoaded)
			NewScene();
	}

	void EditorLayer::SaveProject()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		Ref<Project> activeProject = Project::GetActive();
		if (!activeProject || m_currentProjectPath.empty())
		{
			NewProject();
			return;
		}

		auto& config = activeProject->GetConfig();
		if (!m_editorScene)
		{
			config.StartScene.clear();
		}
		else
		{
			std::filesystem::path sceneSavePath = m_currentScenePath;
			std::filesystem::path relativeScenePath;

			if (sceneSavePath.empty()
				|| !TryGetPathRelativeToDirectory(sceneSavePath, activeProject->GetAssetDirectory(), relativeScenePath))
			{
				sceneSavePath = GetDefaultProjectScenePath(activeProject);
			}

			std::error_code ec;
			std::filesystem::create_directories(sceneSavePath.parent_path(), ec);
			if (ec)
			{
				YUICY_CORE_ERROR("[Project] Failed to create scene directory '{}': {}", sceneSavePath.parent_path().string(), ec.message());
				return;
			}

			SceneSerializer sceneSerializer(m_editorScene);
			sceneSerializer.Serialize(sceneSavePath);
			m_currentScenePath = sceneSavePath.lexically_normal();

			if (TryGetPathRelativeToDirectory(m_currentScenePath, activeProject->GetAssetDirectory(), relativeScenePath))
			{
				config.StartScene = relativeScenePath.generic_string();
			}
			else
			{
				YUICY_CORE_ERROR(
					"[Project] Failed to compute StartScene relative path for '{}' in asset directory '{}'.",
					m_currentScenePath.string(),
					activeProject->GetAssetDirectory().string());
				return;
			}
		}

		ProjectSerializer serializer(activeProject);
		serializer.Serialize(m_currentProjectPath);
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

	std::string EditorLayer::SaveFileDialog(const char* filter, const char* defaultExtension)
	{
		OPENFILENAMEA ofn;
		CHAR szFile[260] = { 0 };
		std::string normalizedExtension;
		ZeroMemory(&ofn, sizeof(OPENFILENAME));
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = glfwGetWin32Window(static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow()));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (defaultExtension && defaultExtension[0] != '\0')
		{
			normalizedExtension = defaultExtension[0] == '.' ? defaultExtension + 1 : defaultExtension;
			ofn.lpstrDefExt = normalizedExtension.c_str();
		}

		if (GetSaveFileNameA(&ofn) == TRUE)
			return ofn.lpstrFile;

		return {};
	}

	void EditorLayer::OnImGuiDrawGizmos()
	{
		if (m_sceneState != SceneState::Edit)
			return;

		Entity selectedEntity = m_sceneHierarchyPanel.GetSelectedEntity();
		if (!selectedEntity || m_gizmoType == -1)
			return;

		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();  // 设置当前绘制队列为 Viewport
		ImGuizmo::SetRect(m_viewportBounds[0].x, m_viewportBounds[0].y, m_viewportSize.x, m_viewportSize.y);

		const glm::mat4& cameraProjection = m_editorCamera.GetProjection();
		glm::mat4 cameraView = m_editorCamera.GetViewMatrix();
		glm::mat4 worldTransform = m_activeScene->GetWorldSpaceTransformMatrix(selectedEntity);

		bool snap = Input::IsKeyPressed(Key::LeftControl) || Input::IsKeyPressed(Key::RightControl);
		float snapValue = 0.5f;
		if (m_gizmoType == ImGuizmo::OPERATION::ROTATE)
			snapValue = 45.0f;
		float snapValues[3] = { snapValue, snapValue, snapValue };  // 变化值

		ImGuizmo::Manipulate(
			glm::value_ptr(cameraView),			// 相机参数
			glm::value_ptr(cameraProjection),
			(ImGuizmo::OPERATION)m_gizmoType,	// 操作类型
			ImGuizmo::LOCAL,					// 本地坐标系
			glm::value_ptr(worldTransform),
			nullptr,
			snap ? snapValues : nullptr
		);

		if (ImGuizmo::IsUsing())  // 正在操作
		{
			glm::mat4 localTransform = worldTransform;
			Entity parent = selectedEntity.GetParent();
			if (parent)
			{
				glm::mat4 parentWorldTransform = m_activeScene->GetWorldSpaceTransformMatrix(parent);
				localTransform = glm::inverse(parentWorldTransform) * worldTransform;
			}

			selectedEntity.GetComponent<TransformComponent>().SetTransform(localTransform);
		}
	}
}
