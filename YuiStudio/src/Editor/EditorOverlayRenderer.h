#pragma once

#include "EditorContext.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy {

	// 编辑器 Overlay 渲染器
	// 只负责绘制编辑器辅助可视化信息，不做业务决策。
	// 根据 EditorContext 中的设置和场景数据决定绘制内容。
	//
	// Phase 1：最小骨架，实现世界原点和选中实体外框
	// Phase 2：扩展 Grid、Camera Bounds、Collider Bounds、Relationship Lines 等
	class EditorOverlayRenderer
	{
	public:
		EditorOverlayRenderer() = default;
		~EditorOverlayRenderer() = default;

		void SetContext(EditorContext* context) { m_context = context; }

		// 主渲染入口 —— 由 EditorRenderPipeline 在 Overlay Pass 中调用
		// 调用者负责 Renderer2D::BeginScene / EndScene
		void Render(const EditorCamera& camera, const Ref<Scene>& scene);

	private:
		// 独立 Overlay 绘制方法
		void DrawWorldOrigin();
		void DrawSelectionBounds(const Ref<Scene>& scene);

		// Phase 2 预留接口（当前不实现）
		// void DrawGrid(const EditorCamera& camera);
		// void DrawCameraBounds(const Ref<Scene>& scene);
		// void DrawColliders(const Ref<Scene>& scene);
		// void DrawRelationshipLines(const Ref<Scene>& scene);

		EditorContext* m_context = nullptr;
	};

}
