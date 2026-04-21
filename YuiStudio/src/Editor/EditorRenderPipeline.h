#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Timestep.h"

#include <cstdint>

namespace Yuicy {

	struct EditorContext;
	class EditorOverlayRenderer;
	class EditorCamera;
	class Framebuffer;
	struct FramebufferSpecification;

	// 编辑器渲染管线
	// 负责调度 Background Overlay → Scene Pass → Foreground Overlay，管理帧缓冲。
	class EditorRenderPipeline
	{
	public:
		EditorRenderPipeline() = default;
		~EditorRenderPipeline() = default;

		void Init(const FramebufferSpecification& spec);

		void SetContext(EditorContext* context) { m_context = context; }
		void SetOverlayRenderer(EditorOverlayRenderer* overlayRenderer) { m_overlayRenderer = overlayRenderer; }

		// 执行完整渲染管线（Scene Pass + Overlay Pass）
		void Execute(Timestep ts, EditorCamera& camera);

		// 视口大小变化
		void OnViewportResize(uint32_t width, uint32_t height);

		// 读取实体 ID
		int ReadEntityIDAtPixel(int x, int y);

		// 获取帧缓冲颜色附件纹理 ID
		uint32_t GetColorAttachmentRendererID() const;

		// 获取帧缓冲
		Ref<Framebuffer> GetFramebuffer() const { return m_framebuffer; }

	private:
		void ExecuteBackgroundOverlayPass(EditorCamera& camera);
		void ExecuteScenePass(Timestep ts, EditorCamera& camera);
		void ExecuteForegroundOverlayPass(EditorCamera& camera);

		EditorContext* m_context = nullptr;
		EditorOverlayRenderer* m_overlayRenderer = nullptr;
		Ref<Framebuffer> m_framebuffer;
	};

}
