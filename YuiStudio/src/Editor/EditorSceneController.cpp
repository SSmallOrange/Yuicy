#include "pch.h"

#include "EditorSceneController.h"
#include "EditorContext.h"
#include "EditorDirtyTracker.h"

#include "Yuicy/Scene/SceneSerializer.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Project/ProjectSerializer.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Core/Application.h"

#include "imgui/imgui.h"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <commdlg.h>

namespace Yuicy {

	// 静态工具函数
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

	static bool TryGetPathRelativeToDirectory(const std::filesystem::path& filepath, const std::filesystem::path& directory,
		std::filesystem::path& outRelativePath)
	{
		if (filepath.empty() || directory.empty())
			return false;

		std::filesystem::path relativePath = filepath.lexically_normal().lexically_relative(directory.lexically_normal());
		if (relativePath.empty())
			return false;

		if (auto it = relativePath.begin(); it != relativePath.end() && it->string() == "..")
			return false;

		outRelativePath = relativePath;
		return true;
	}

	// 回调通知
	void EditorSceneController::NotifySceneChanged()
	{
		if (m_onSceneChanged)
			m_onSceneChanged();
	}

	// Dirty 检查与确认流程
	bool EditorSceneController::CheckDirtyAndConfirm(PendingAction action)
	{
		if (!m_dirtyTracker || !m_dirtyTracker->IsSceneDirty())
			return true; // 无需确认，可以直接执行

		m_pendingAction = action;
		m_showUnsavedDialog = true;
		return false; // 需要等待用户确认
	}

	void EditorSceneController::ExecutePendingAction()
	{
		PendingAction action = m_pendingAction;
		std::filesystem::path filepath = m_pendingFilePath;
		m_pendingAction = PendingAction::None;
		m_pendingFilePath.clear();

		switch (action)
		{
		case PendingAction::NewScene:     NewScene(); break;
		case PendingAction::OpenScene:    OpenScene(filepath); break;
		case PendingAction::OpenProject:  OpenProject(filepath); break;
		case PendingAction::NewProject:   NewProject(); break;
		default: break;
		}
	}

	void EditorSceneController::OnImGuiRender()
	{
		if (m_showUnsavedDialog)
		{
			ImGui::OpenPopup("Unsaved Changes##SceneController");
			m_showUnsavedDialog = false;

			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}

		if (!ImGui::BeginPopupModal("Unsaved Changes##SceneController", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			return;

		ImGui::Text("The current scene has unsaved changes.");
		ImGui::Text("Do you want to save before continuing?");
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		float buttonWidth = 100.0f;
		float totalWidth = buttonWidth * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

		if (ImGui::Button("Save", ImVec2(buttonWidth, 0)))
		{
			SaveScene();
			ExecutePendingAction();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Don't Save", ImVec2(buttonWidth, 0)))
		{
			if (m_dirtyTracker)
				m_dirtyTracker->ClearSceneDirty();
			ExecutePendingAction();
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
		{
			m_pendingAction = PendingAction::None;
			m_pendingFilePath.clear();
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// 场景操作
	void EditorSceneController::NewScene()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		if (m_pendingAction == PendingAction::None && !CheckDirtyAndConfirm(PendingAction::NewScene))
			return;

		m_context->editorScene = CreateRef<Scene>();
		CreateDefaultSceneContent(m_context->editorScene);

		auto& viewportState = m_context->viewport;
		m_context->editorScene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);
		m_context->activeScene = m_context->editorScene;
		m_context->document.currentScenePath = std::filesystem::path{};
		m_context->viewport.hoveredEntity = {};
		m_context->selection.ClearEntitySelection();
		m_context->ClearTilemapSceneState();

		if (m_dirtyTracker)
			m_dirtyTracker->ClearSceneDirty();

		NotifySceneChanged();
	}

	void EditorSceneController::OpenSceneDialog()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		std::string filepath = ShowOpenFileDialog(SceneSerializer::GetSceneSerializerFileFilter());
		if (!filepath.empty())
		{
			m_pendingFilePath = filepath;
			if (CheckDirtyAndConfirm(PendingAction::OpenScene))
			{
				m_pendingFilePath.clear();
				OpenScene(filepath);
			}
		}
	}

	bool EditorSceneController::OpenScene(const std::filesystem::path& filepath)
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return false;

		// 从 ExecutePendingAction 调用时不再检查 dirty
		if (m_pendingAction == PendingAction::None && !CheckDirtyAndConfirm(PendingAction::OpenScene))
		{
			m_pendingFilePath = filepath;
			return false;
		}

		Ref<Scene> scene = CreateRef<Scene>();

		SceneSerializer serializer(scene);
		if (!serializer.Deserialize(filepath))
			return false;

		auto& viewportState = m_context->viewport;
		scene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);

		m_context->editorScene = scene;
		m_context->activeScene = m_context->editorScene;
		m_context->document.currentScenePath = filepath.lexically_normal();
		m_context->viewport.hoveredEntity = {};
		m_context->selection.ClearEntitySelection();
		m_context->ClearTilemapSceneState();

		if (m_dirtyTracker)
			m_dirtyTracker->ClearSceneDirty();

		NotifySceneChanged();
		return true;
	}

	void EditorSceneController::SaveScene()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		if (!m_context->document.currentScenePath.empty())
		{
			SceneSerializer serializer(m_context->editorScene);
			serializer.Serialize(m_context->document.currentScenePath);

			if (m_dirtyTracker)
			{
				m_dirtyTracker->ClearSceneDirty();
				m_dirtyTracker->ClearProjectDirty();
			}

			if (Project::GetActive())
				SaveProject();
		}
		else
		{
			SaveSceneAs();
		}
	}

	void EditorSceneController::SaveSceneAs()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		std::string filepath = ShowSaveFileDialog(
			SceneSerializer::GetSceneSerializerFileFilter(),
			SceneSerializer::GetSceneSerializerDefaultExtension());
		if (!filepath.empty())
		{
			std::filesystem::path scenePath = filepath;
			if (scenePath.extension() != SceneSerializer::GetSceneSerializerDefaultExtension())
				scenePath += SceneSerializer::GetSceneSerializerDefaultExtension();

			SceneSerializer serializer(m_context->editorScene);
			serializer.Serialize(scenePath);
			m_context->document.currentScenePath = scenePath;

			if (m_dirtyTracker)
			{
				m_dirtyTracker->ClearSceneDirty();
				m_dirtyTracker->ClearProjectDirty();
			}

			if (Project::GetActive())
				SaveProject();
		}
	}

	// 项目操作
	void EditorSceneController::NewProject()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		if (m_pendingAction == PendingAction::None && !CheckDirtyAndConfirm(PendingAction::NewProject))
			return;

		std::string filepath = ShowSaveFileDialog(
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
		m_context->document.currentProjectPath = projectPath.lexically_normal();

		if (!m_context->editorScene)
			NewScene();

		SaveProject();
	}

	void EditorSceneController::OpenProjectDialog()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		std::string filepath = ShowOpenFileDialog(ProjectSerializer::GetProjectSerializerFileFilter());
		if (!filepath.empty())
		{
			m_pendingFilePath = filepath;
			if (CheckDirtyAndConfirm(PendingAction::OpenProject))
			{
				m_pendingFilePath.clear();
				OpenProject(filepath);
			}
		}
	}

	void EditorSceneController::OpenProject(const std::filesystem::path& filepath)
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		auto project = CreateRef<Project>();
		ProjectSerializer serializer(project);
		if (!serializer.Deserialize(filepath))
			return;

		Project::SetActive(project);
		m_context->document.currentProjectPath = filepath;

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

	void EditorSceneController::SaveProject()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		Ref<Project> activeProject = Project::GetActive();
		if (!activeProject || m_context->document.currentProjectPath.empty())
		{
			NewProject();
			return;
		}

		auto& config = activeProject->GetConfig();
		if (!m_context->editorScene)
		{
			config.StartScene.clear();
		}
		else
		{
			std::filesystem::path sceneSavePath = m_context->document.currentScenePath;
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

			SceneSerializer sceneSerializer(m_context->editorScene);
			sceneSerializer.Serialize(sceneSavePath);
			m_context->document.currentScenePath = sceneSavePath.lexically_normal();

			if (TryGetPathRelativeToDirectory(m_context->document.currentScenePath, activeProject->GetAssetDirectory(), relativeScenePath))
			{
				config.StartScene = relativeScenePath.generic_string();
			}
			else
			{
				YUICY_CORE_ERROR(
					"[Project] Failed to compute StartScene relative path for '{}' in asset directory '{}'.",
					m_context->document.currentScenePath.string(),
					activeProject->GetAssetDirectory().string());
				return;
			}
		}

		ProjectSerializer projectSerializer(activeProject);
		projectSerializer.Serialize(m_context->document.currentProjectPath);

		if (m_dirtyTracker)
			m_dirtyTracker->ClearProjectDirty();
	}

	// Play / Stop / Simulate
	void EditorSceneController::OnScenePlay()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		if (!SceneHasPrimaryCamera(m_context->editorScene))
		{
			YUICY_CORE_WARN("Cannot enter Play mode: scene has no primary camera.");
			return;
		}

		m_context->runtime.mode = SceneMode::Play;
		m_context->runtime.paused = false;
		m_context->runtime.pendingStepFrames = 0;
		m_context->EndTilemapBrushStroke();
		m_context->tilemap.ClearHover();

		m_context->runtimeScene = Scene::Copy(m_context->editorScene);
		auto& viewportState = m_context->viewport;
		m_context->runtimeScene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);
		m_context->runtimeScene->OnRuntimeStart();  // 场景运行初始化

		m_context->activeScene = m_context->runtimeScene;
		m_context->viewport.hoveredEntity = {};

		NotifySceneChanged();
	}

	void EditorSceneController::OnSceneSimulate()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		m_context->runtime.mode = SceneMode::Simulate;
		m_context->runtime.paused = false;
		m_context->runtime.pendingStepFrames = 0;
		m_context->EndTilemapBrushStroke();
		m_context->tilemap.ClearHover();

		m_context->runtimeScene = Scene::Copy(m_context->editorScene);
		auto& viewportState = m_context->viewport;
		m_context->runtimeScene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);
		m_context->runtimeScene->OnSimulationStart();

		m_context->activeScene = m_context->runtimeScene;
		m_context->viewport.hoveredEntity = {};

		NotifySceneChanged();
	}

	void EditorSceneController::OnSceneStop()
	{
		if (!m_context || m_context->runtime.IsEditing())
			return;

		if (m_context->runtime.mode == SceneMode::Play)
		{
			if (m_context->activeScene)
				m_context->activeScene->OnRuntimeStop();
		}
		else if (m_context->runtime.mode == SceneMode::Simulate)
		{
			if (m_context->activeScene)
				m_context->activeScene->OnSimulationStop();
		}

		m_context->runtime.mode = SceneMode::Edit;
		m_context->runtime.paused = false;
		m_context->runtime.pendingStepFrames = 0;
		m_context->runtimeScene = nullptr;
		m_context->EndTilemapBrushStroke();
		m_context->tilemap.ClearHover();

		m_context->activeScene = m_context->editorScene;
		auto& viewportState = m_context->viewport;
		m_context->activeScene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);
		m_context->viewport.hoveredEntity = {};

		NotifySceneChanged();
	}

	void EditorSceneController::OnScenePause()
	{
		if (!m_context || m_context->runtime.IsEditing())
			return;

		m_context->runtime.paused = !m_context->runtime.paused;
	}

	void EditorSceneController::OnSceneStep()
	{
		if (!m_context || m_context->runtime.IsEditing())
			return;

		// Step 只有在暂停状态下才生效
		m_context->runtime.paused = true;
		m_context->runtime.pendingStepFrames = 1;
	}

	// 文件对话框 (Win32)
	std::string EditorSceneController::ShowOpenFileDialog(const char* filter)
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

	std::string EditorSceneController::ShowSaveFileDialog(const char* filter, const char* defaultExtension)
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

}
