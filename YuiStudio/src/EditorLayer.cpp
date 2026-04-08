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

		// 默认相机
		auto cameraEntity = m_editorScene->CreateEntity("EditorCamera");
		cameraEntity.AddComponent<CameraComponent>();

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
			m_activeScene->OnUpdateEditor(ts);
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

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;  // 设置顶层窗口为带菜单、且没有Dock属性的窗口

		// GetMainViewport() 返回操作系统窗口的可用区域信息
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);					// 窗口位置 = 屏幕左上角
		ImGui::SetNextWindowSize(viewport->WorkSize);				// 窗口大小 = 整个屏幕
		ImGui::SetNextWindowViewport(viewport->ID);					// 绑定到主视口

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
		ImGui::PopStyleVar(3); // 弹出前面 Push 的 3 个 StyleVar

		// --- 在 DockSpace 窗口内部创建停靠区域 ---
		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		float minWinSizeX = style.WindowMinSize.x;
		style.WindowMinSize.x = 370.0f;  // 防止子窗口被拖拽得太小

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			// GetID("名称") → 生成一个唯一的哈希值作为 Dockspace 的标识
			ImGuiID dockspaceId = ImGui::GetID("YuiStudioDockspace");
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		style.WindowMinSize.x = minWinSizeX; // 恢复原始最小宽度

		// 菜单栏
		UIMenuBar();

		// Viewport 面板
		OnImGuiViewportRender();
		// State 面板
		OnImGuiDrawStateRender();

		// Scene Hierarchy + Properties
		m_sceneHierarchyPanel.OnImGuiRender();

		ImGui::End(); // 结束 DockSpace
	}

	void EditorLayer::OnImGuiViewportRender()
	{
		// WindowPadding = 0 → Viewport 内容（图片）完全贴边，没有留白
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		// 检查当前窗口的焦点/悬停状态
		m_viewportFocused = ImGui::IsWindowFocused();  // 此窗口是否获得键盘焦点
		m_viewportHovered = ImGui::IsWindowHovered();  // 鼠标是否悬停在此窗口内

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

	void EditorLayer::UIMenuBar()
	{
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
	}

	void EditorLayer::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

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
