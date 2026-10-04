#include "pch.h"

#include "EditorSceneController.h"
#include "EditorContext.h"
#include "EditorDirtyTracker.h"
#include "EditorProjectSession.h"

#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Scene/SceneSerializer.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Project/ProjectSerializer.h"
#include "Yuicy/Core/Log.h"
#include "Yuicy/Core/FileDialogs.h"

#include "imgui/imgui.h"

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

	static std::filesystem::path GetDefaultProjectScenePath(const Project& project)
	{
		return project.GetAssetDirectory() / "Scenes" / ("StartScene" + std::string(SceneSerializer::GetSceneSerializerDefaultExtension()));
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

	void EditorSceneController::AttachSceneContext(const Ref<Scene>& scene) const
	{
		if (!scene || !m_context)
			return;

		scene->SetContext(m_context->project ? m_context->project->MakeSceneContext() : SceneContext{});
	}

	Ref<Scene> EditorSceneController::CreateDefaultScene() const
	{
		Ref<Scene> scene = CreateRef<Scene>();
		AttachSceneContext(scene);
		CreateDefaultSceneContent(scene);
		return scene;
	}

	void EditorSceneController::SetEditorScene(const Ref<Scene>& scene, const std::filesystem::path& scenePath)
	{
		auto& viewportState = m_context->viewport;
		scene->OnViewportResize((uint32_t)viewportState.size.x, (uint32_t)viewportState.size.y);

		m_context->editorScene = scene;
		m_context->activeScene = m_context->editorScene;
		m_context->document.currentScenePath = scenePath;
		m_context->viewport.hoveredEntity = {};
		m_context->selection.ClearEntitySelection();

		if (m_dirtyTracker)
			m_dirtyTracker->ClearSceneDirty();

		NotifySceneChanged();
	}

	void EditorSceneController::WriteCurrentAssetRegistry() const
	{
		if (EditorAssetManager* assetManager = m_context->GetAssetManager())
			assetManager->WriteRegistryToFile();
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

		SetEditorScene(CreateDefaultScene(), {});
	}

	void EditorSceneController::OpenSceneDialog()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		std::optional<std::filesystem::path> filepath = FileDialogs::OpenFile(SceneSerializer::GetSceneSerializerFileFilter());
		if (filepath)
		{
			m_pendingFilePath = *filepath;
			if (CheckDirtyAndConfirm(PendingAction::OpenScene))
			{
				m_pendingFilePath.clear();
				OpenScene(*filepath);
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

		AttachSceneContext(scene);
		SetEditorScene(scene, filepath.lexically_normal());
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

			if (m_context->project)
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

		std::optional<std::filesystem::path> filepath = FileDialogs::SaveFile(SceneSerializer::GetSceneSerializerFileFilter());
		if (filepath)
		{
			std::filesystem::path scenePath = *filepath;
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

			if (m_context->project)
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

		std::optional<std::filesystem::path> filepath = FileDialogs::SaveFile(ProjectSerializer::GetProjectSerializerFileFilter());
		if (!filepath)
			return;

		std::filesystem::path projectPath = *filepath;
		if (projectPath.extension() != ProjectSerializer::GetProjectSerializerDefaultExtension())
			projectPath += ProjectSerializer::GetProjectSerializerDefaultExtension();

		WriteCurrentAssetRegistry();
		Scope<EditorProjectSession> session = EditorProjectSession::Create(projectPath);
		if (!session)
			return;

		m_context->project = std::move(session);
		m_context->selection.ClearAssetSelection();
		AttachSceneContext(m_context->editorScene);

		if (!m_context->editorScene)
			NewScene();

		SaveProject();
	}

	void EditorSceneController::OpenProjectDialog()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		std::optional<std::filesystem::path> filepath = FileDialogs::OpenFile(ProjectSerializer::GetProjectSerializerFileFilter());
		if (filepath)
		{
			m_pendingFilePath = *filepath;
			if (CheckDirtyAndConfirm(PendingAction::OpenProject))
			{
				m_pendingFilePath.clear();
				OpenProject(*filepath);
			}
		}
	}

	void EditorSceneController::OpenProject(const std::filesystem::path& filepath)
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		WriteCurrentAssetRegistry();
		Scope<EditorProjectSession> session = EditorProjectSession::Open(filepath);
		if (!session)
			return;

		m_context->project = std::move(session);
		m_context->selection.ClearAssetSelection();

		// 不调用 OpenScene：它会再做一次未保存检查，而调用方在切换项目前已经确认过
		Ref<Scene> startScene;
		std::filesystem::path startScenePath;
		const Project& project = m_context->project->GetProject();
		if (!project.GetSettings().StartScene.empty())
		{
			startScenePath = (project.GetAssetDirectory() / project.GetSettings().StartScene).lexically_normal();
			if (std::filesystem::exists(startScenePath))
			{
				startScene = CreateRef<Scene>();
				if (!SceneSerializer(startScene).Deserialize(startScenePath))
					startScene = nullptr;
			}
			else
			{
				YUICY_CORE_WARN("[Project] Start scene not found: {}", startScenePath.string());
			}
		}

		if (startScene)
		{
			AttachSceneContext(startScene);
			SetEditorScene(startScene, startScenePath);
		}
		else
		{
			SetEditorScene(CreateDefaultScene(), {});
		}
	}

	void EditorSceneController::SaveProject()
	{
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		EditorProjectSession* session = m_context->project.get();
		if (!session)
		{
			NewProject();
			return;
		}

		Project& project = session->GetProject();
		ProjectSettings& settings = project.GetSettings();
		if (!m_context->editorScene)
		{
			settings.StartScene.clear();
		}
		else
		{
			std::filesystem::path sceneSavePath = m_context->document.currentScenePath;
			std::filesystem::path relativeScenePath;

			if (sceneSavePath.empty()
				|| !TryGetPathRelativeToDirectory(sceneSavePath, project.GetAssetDirectory(), relativeScenePath))
			{
				sceneSavePath = GetDefaultProjectScenePath(project);
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

			if (TryGetPathRelativeToDirectory(m_context->document.currentScenePath, project.GetAssetDirectory(), relativeScenePath))
			{
				settings.StartScene = relativeScenePath;
			}
			else
			{
				YUICY_CORE_ERROR(
					"[Project] Failed to compute StartScene relative path for '{}' in asset directory '{}'.",
					m_context->document.currentScenePath.string(),
					project.GetAssetDirectory().string());
				return;
			}
		}

		if (!session->Save())
			return;

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

}
