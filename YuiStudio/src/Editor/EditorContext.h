#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/UUID.h"
#include "Yuicy/Asset/Asset.h"
#include "Yuicy/Scene/Scene.h"
#include "Yuicy/Scene/Entity.h"
#include "Yuicy/Tilemap/TilemapTypes.h"

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

	enum class TilemapTool
	{
		Select = 0,
		Move,
		Paint,
		Erase,
		BoxFill,
		FloodFill,
		Picker
	};

	// Tilemap 创作工具共享状态
	struct EditorTilemapState
	{
		AssetHandle m_activePalette = 0;
		AssetHandle m_activeTile = 0;
		UUID m_activeTilemapEntity = 0;
		TilemapTool m_activeTool = TilemapTool::Paint;
		GridPosition m_hoveredCell;
		bool m_hasHoveredCell = false;
		bool m_showGridOverlay = true;
		bool m_isPainting = false;
		GridPosition m_boxFillStartCell;
		GridPosition m_boxFillEndCell;
		bool m_hasBoxFillPreview = false;
		bool m_boxFillErases = false;

		void ClearHover()
		{
			m_hoveredCell = {};
			m_hasHoveredCell = false;
		}

		void EndBrushStroke()
		{
			m_isPainting = false;
			ClearBoxFillPreview();
		}

		void SetBoxFillPreview(const GridPosition& startCell, const GridPosition& endCell, bool erases)
		{
			m_boxFillStartCell = startCell;
			m_boxFillEndCell = endCell;
			m_hasBoxFillPreview = true;
			m_boxFillErases = erases;
		}

		void ClearBoxFillPreview()
		{
			m_boxFillStartCell = {};
			m_boxFillEndCell = {};
			m_hasBoxFillPreview = false;
			m_boxFillErases = false;
		}

		void ClearSceneState()
		{
			m_activeTilemapEntity = 0;
			ClearHover();
			EndBrushStroke();
		}
	};

	// 编辑器实体元数据（不污染运行时 ECS）
	struct EditorEntityMetadata
	{
		bool locked = false;   // 锁定：不可选中、Gizmo 不可操作
		bool hidden = false;   // 隐藏：不可选中
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
		EditorTilemapState      tilemap;

		// 实体编辑器元数据
		std::unordered_map<UUID, EditorEntityMetadata> entityMetadata;

		bool IsEntityLocked(UUID uuid) const
		{
			auto it = entityMetadata.find(uuid);
			return it != entityMetadata.end() && it->second.locked;
		}

		bool IsEntityHidden(UUID uuid) const
		{
			auto it = entityMetadata.find(uuid);
			return it != entityMetadata.end() && it->second.hidden;
		}

		// 递归检查隐藏情况
		bool IsEntityHiddenInHierarchy(UUID uuid) const
		{
			if (!activeScene) return false;
			UUID current = uuid;
			while (current != 0)
			{
				if (IsEntityHidden(current))
					return true;
				Entity entity = activeScene->FindEntityByUUID(current);
				if (!entity) break;
				current = entity.GetParentUUID();
			}
			return false;
		}

		bool IsEntitySelectable(UUID uuid) const
		{
			return !IsEntityLocked(uuid) && !IsEntityHiddenInHierarchy(uuid);
		}

		void ToggleEntityLocked(UUID uuid)
		{
			entityMetadata[uuid].locked = !entityMetadata[uuid].locked;
		}

		void ToggleEntityHidden(UUID uuid)
		{
			entityMetadata[uuid].hidden = !entityMetadata[uuid].hidden;
		}

		void ClearEntityMetadata()
		{
			entityMetadata.clear();
		}

		void ClearTilemapSceneState()
		{
			tilemap.ClearSceneState();
		}

		void EndTilemapBrushStroke()
		{
			tilemap.EndBrushStroke();
		}

		void ValidateTilemapState()
		{
			if (!activeScene)
			{
				tilemap.ClearSceneState();
				return;
			}

			if (tilemap.m_activeTilemapEntity == 0)
				return;

			Entity activeTilemapEntity = activeScene->FindEntityByUUID(tilemap.m_activeTilemapEntity);
			if (!activeTilemapEntity || !activeTilemapEntity.HasComponent<TilemapComponent>())
				tilemap.ClearSceneState();
		}
	};

}
