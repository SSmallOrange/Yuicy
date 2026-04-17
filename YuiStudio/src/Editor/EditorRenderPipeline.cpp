#include "EditorRenderPipeline.h"
#include "EditorOverlayRenderer.h"

#include "Yuicy/Renderer/Renderer2D.h"
#include "Yuicy/Renderer/RenderCommand.h"

namespace Yuicy {

	void EditorRenderPipeline::Init(const FramebufferSpecification& spec)
	{
		m_framebuffer = Framebuffer::Create(spec);
	}

	void EditorRenderPipeline::Execute(Timestep ts, EditorCamera& camera)
	{
		if (!m_context || !m_context->activeScene)
			return;

		Renderer2D::ResetStats();

		m_framebuffer->Bind();

		RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
		RenderCommand::Clear();

		// 清除实体 ID 附件为 -1（无实体）
		m_framebuffer->ClearAttachment(1, -1);

		// Scene Pass：渲染场景内容，写入 Color + EntityID
		ExecuteScenePass(ts, camera);

		// Overlay Pass：非 Play 模式下渲染
		if (m_context->runtime.mode != SceneMode::Play)
			ExecuteOverlayPass(camera);

		m_framebuffer->Unbind();
	}

	void EditorRenderPipeline::ExecuteScenePass(Timestep ts, EditorCamera& camera)
	{
		switch (m_context->runtime.mode)
		{
		case SceneMode::Edit:
			m_context->activeScene->OnUpdateEditor(ts, camera);
			break;
		case SceneMode::Play:
			m_context->activeScene->OnUpdateRuntime(ts);
			break;
		case SceneMode::Simulate:
			// TODO: Simulate 模式更新
			m_context->activeScene->OnUpdateEditor(ts, camera);
			break;
		}
	}

	void EditorRenderPipeline::ExecuteOverlayPass(EditorCamera& camera)
	{
		if (!m_overlayRenderer)
			return;

		// 使用编辑器相机开始新的渲染批次
		Renderer2D::BeginScene(camera);
		m_overlayRenderer->Render(camera, m_context->activeScene);
		Renderer2D::EndScene();
	}

	void EditorRenderPipeline::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		auto spec = m_framebuffer->GetSpecification();
		if (spec.width != width || spec.height != height)
		{
			m_framebuffer->Resize(width, height);
		}
	}

	int EditorRenderPipeline::ReadEntityIDAtPixel(int x, int y)
	{
		m_framebuffer->Bind();
		int result = m_framebuffer->ReadPixel(1, x, y);
		m_framebuffer->Unbind();
		return result;
	}

	uint32_t EditorRenderPipeline::GetColorAttachmentRendererID() const
	{
		return m_framebuffer->GetColorAttachmentRendererID();
	}

}
