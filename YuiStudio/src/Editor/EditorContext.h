#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"

#include "EditorSelectionContext.h"
#include "EditorViewportSettings.h"
#include "EditorSettings.h"

#include <glm/glm.hpp>
#include <array>
#include <filesystem>

namespace Yuicy {

	// 场景运行态状态：主模式 + 暂停/单步 附加控制
	enum class SceneMode
	{
		Edit = 0,
		Play,
		Simulate
	};

	struct EditorSceneRuntimeState
	{
		SceneMode mode = SceneMode::Edit;
		bool paused = false;
		uint32_t pendingStepFrames = 0;

		bool IsEditing() const { return mode == SceneMode::Edit; }
		bool IsRunning() const { return mode == SceneMode::Play || mode == SceneMode::Simulate; }
	};

	// 文档状态：当前路径、脏标记、保存时间等
	struct EditorDocumentState
	{
		std::filesystem::path currentScenePath;
		std::filesystem::path currentProjectPath;

		bool sceneDirty   = false;	// 是否脏数据
		bool projectDirty = false;

		double lastSaveTime     = 0.0;
		double lastAutoSaveTime = 0.0;
	};

	// 视口运行时状态
	struct EditorViewportState
	{
		glm::vec2 size = { 0.0f, 0.0f };
		std::array<glm::vec2, 2> bounds = {};  // 视口坐标（ImGui）

		bool focused = false;
		bool hovered = false;

		Entity hoveredEntity;
	};

	// 编辑器全局共享上下文，所有面板和服务从此处读写状态
 	struct EditorContext
	{
		// 场景
		Ref<Scene> editorScene;
		Ref<Scene> runtimeScene;
		Ref<Scene> activeScene;

		// 子状态
		EditorSceneRuntimeState runtime;
		EditorDocumentState     document;
		EditorSelectionContext  selection;
		EditorViewportState     viewport;
		EditorViewportSettings  viewportSettings;
		EditorSettings          settings;
	};

}
