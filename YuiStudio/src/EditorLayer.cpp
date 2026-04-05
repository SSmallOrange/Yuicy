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
		// ========================================================================
		// Dockspace 设置
		// ========================================================================
		// Dockspace 是 ImGui 的"停靠空间"，相当于一个容器窗口。
		// 所有子窗口（Viewport、Stats 等）都可以拖拽停靠在它内部。
		// 我们要把它设置成全屏、不可移动的背景窗口。

		static bool dockspaceOpen = true;
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;  // 设置顶层窗口为带菜单、且没有Dock属性的窗口

		// GetMainViewport() 返回操作系统窗口的可用区域信息
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);      // 窗口位置 = 屏幕左上角
		ImGui::SetNextWindowSize(viewport->WorkSize);    // 窗口大小 = 整个屏幕
		ImGui::SetNextWindowViewport(viewport->ID);      // 绑定到主视口

		// PushStyleVar / PopStyleVar 是临时修改 ImGui 样式的方式
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);    // 窗口圆角 = 0（方角）
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);  // 窗口边框 = 0（无边框）

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
			// DockSpace() 在当前窗口内创建一个可停靠区域
			// 之后所有 Begin/End 的子窗口都可以拖拽停靠到这个区域内
			ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
		}

		style.WindowMinSize.x = minWinSizeX; // 恢复原始最小宽度

		// 菜单栏（因为 DockSpace 窗口设置了 MenuBar 标志，所以菜单栏会出现在它的顶部）
		UIMenuBar();

		// Viewport 面板

		// WindowPadding = 0 → Viewport 内容（图片）完全贴边，没有留白
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
		ImGui::Begin("Viewport");

		// 检查当前窗口的焦点/悬停状态（用于决定是否响应快捷键和鼠标事件）
		m_viewportFocused = ImGui::IsWindowFocused(); // 此窗口是否获得键盘焦点
		m_viewportHovered = ImGui::IsWindowHovered();  // 鼠标是否悬停在此窗口内

		// GetContentRegionAvail() → 当前窗口内"可用区域"的尺寸（减去标题栏、边距等）
		// 当用户拖拽调整 Viewport 面板大小时，这个值会变化
		// 我们用它来决定 Framebuffer 的分辨率（下一帧在 OnUpdate 中 Resize）
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		m_viewportSize = { viewportPanelSize.x, viewportPanelSize.y };

		// 获取 Framebuffer 颜色附件的 OpenGL 纹理 ID
		uint64_t textureID = m_framebuffer->GetColorAttachmentRendererID();

		// ImGui::Image() → 在窗口内绘制一张纹理
		// 参数1: 纹理ID（void* 类型，OpenGL纹理ID强转过来）
		// 参数2: 显示尺寸（填满整个可用区域）
		// 参数3: UV 左上角 = (0,1) ← 注意 Y 翻转！
		// 参数4: UV 右下角 = (1,0)
		// OpenGL 纹理的原点在左下角，而 ImGui 的原点在左上角，所以 UV 的 Y 要翻转
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ m_viewportSize.x, m_viewportSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });

		ImGui::End(); // 结束 Viewport 窗口
		ImGui::PopStyleVar(); // 恢复 WindowPadding

		// Stats 面板 — 渲染统计信息
		ImGui::Begin("Stats");

		auto stats = Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats:");            // Text() → 显示一行纯文本
		ImGui::Text("Draw Calls: %d", stats.DrawCalls);  // 支持 printf 风格格式化
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());
		ImGui::Separator();                          // Separator() → 画一条水平分割线
		ImGui::Text("Scene: %s", m_activeScene->GetName().c_str());

		ImGui::End(); // 结束 Stats 窗口

		ImGui::End(); // 结束 DockSpace
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
