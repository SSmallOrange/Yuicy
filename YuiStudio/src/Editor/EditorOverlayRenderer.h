#pragma once

#include "EditorContext.h"
#include "Yuicy/Renderer/EditorCamera.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy {

	// 编辑器 Overlay 渲染器
	// 只负责绘制编辑器辅助可视化信息，不做业务决策。
	// 根据 EditorContext 中的设置和场景数据决定绘制内容。
	class EditorOverlayRenderer
	{
	public:
		EditorOverlayRenderer() = default;
		~EditorOverlayRenderer() = default;

		void SetContext(EditorContext* context) { m_context = context; }

		// 主渲染入口 —— 由 EditorRenderPipeline 在 Overlay Pass 中调用
		// 调用者负责 Renderer2D::BeginScene / EndScene
		// 背景层 绘制网格、世界原点等需要位于场景内容下方的辅助信息
		void RenderBackground(const EditorCamera& camera, const Ref<Scene>& scene);

		// 前景层 绘制选中框、相机范围、碰撞体等需要覆盖在场景内容上方的辅助信息
		void RenderForeground(const EditorCamera& camera, const Ref<Scene>& scene);

	private:
		// 背景层 Overlay
		void DrawGrid(const EditorCamera& camera);
		void DrawWorldOrigin(const EditorCamera& camera);

		// 前景层 Overlay
		void DrawSelectionBounds(const Ref<Scene>& scene);
		void DrawCameraBounds(const Ref<Scene>& scene);
		void DrawColliders(const Ref<Scene>& scene);

		void DrawEntityPivot(const Ref<Scene>& scene);
		void DrawRelationshipLines(const Ref<Scene>& scene);

		EditorContext* m_context = nullptr;
	};

}
