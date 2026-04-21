#include "pch.h"

#include "EditorRenderPipeline.h"
#include "EditorContext.h"
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

		// Background Overlay Pass
		if (m_context->runtime.mode != SceneMode::Play)
			ExecuteBackgroundOverlayPass(camera);

		// Scene Pass：渲染场景内容
		ExecuteScenePass(ts, camera);

		// Foreground Overlay Pass
		if (m_context->runtime.mode != SceneMode::Play)
			ExecuteForegroundOverlayPass(camera);

		m_framebuffer->Unbind();
	}

	void EditorRenderPipeline::ExecuteBackgroundOverlayPass(EditorCamera& camera)
	{
		if (!m_overlayRenderer)
			return;

		Renderer2D::BeginScene(camera);
		m_overlayRenderer->RenderBackground(camera, m_context->activeScene);
		Renderer2D::EndScene();
	}

	void EditorRenderPipeline::ExecuteScenePass(Timestep ts, EditorCamera& camera)
	{
		auto& runtime = m_context->runtime;

		// Pause / Step 时间控制
		Timestep effectiveTs = ts;
		if (runtime.IsRunning() && runtime.paused)
		{
			if (runtime.pendingStepFrames > 0)
			{
				runtime.pendingStepFrames--;
			}
			else
			{
				effectiveTs = Timestep(0.0f);
			}
		}

		// 编辑器实体可见性过滤
		auto entityFilter = [this](entt::entity e) -> bool {
			Entity entity(e, m_context->activeScene.get());
			if (entity && entity.HasComponent<IDComponent>())
				return !m_context->IsEntityHidden(entity.GetUUID());
			return true;
		};

		switch (runtime.mode)
		{
		case SceneMode::Edit:
			m_context->activeScene->OnUpdateEditor(ts, camera, entityFilter);
			break;
		case SceneMode::Play:
			m_context->activeScene->OnUpdateRuntime(effectiveTs);
			break;
		case SceneMode::Simulate:
			m_context->activeScene->OnUpdateSimulation(effectiveTs, camera, entityFilter);
			break;
		}
	}

	void EditorRenderPipeline::ExecuteForegroundOverlayPass(EditorCamera& camera)
	{
		if (!m_overlayRenderer)
			return;

		// 关闭深度测试，确保 Overlay 置顶
		RenderCommand::SetDepthTest(false);

		Renderer2D::BeginScene(camera);
		m_overlayRenderer->RenderForeground(camera, m_context->activeScene);
		Renderer2D::EndScene();

		RenderCommand::SetDepthTest(true);
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
