#include "pch.h"
#include "PostProcessPipeline.h"

#include "Yuicy/Renderer/RenderCommand.h"

namespace Yuicy {

	void PostProcessPipeline::Init(uint32_t width, uint32_t height)
	{
		if (m_initialized)
			return;

		m_width = width;
		m_height = height;

		CreatePingPongFBOs(width, height);
		CreateFullscreenQuad();

		m_initialized = true;
		YUICY_CORE_INFO("PostProcessPipeline: Initialized ({}x{})", width, height);
	}

	void PostProcessPipeline::Shutdown()
	{
		for (auto& pass : m_passes)
			pass->Shutdown();
		m_passes.clear();

		m_pingFBO = nullptr;
		m_pongFBO = nullptr;
		m_quadVAO = nullptr;
		m_quadVBO = nullptr;
		m_initialized = false;
	}

	void PostProcessPipeline::Resize(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
			return;

		m_width = width;
		m_height = height;

		if (m_pingFBO) m_pingFBO->Resize(width, height);
		if (m_pongFBO) m_pongFBO->Resize(width, height);
	}

	void PostProcessPipeline::AddPass(Ref<IPostProcessPass> pass)
	{
		pass->SetSharedQuadVAO(m_quadVAO);
		m_passes.push_back(pass);
	}

	void PostProcessPipeline::RemovePass(const std::string& name)
	{
		m_passes.erase(
			std::remove_if(m_passes.begin(), m_passes.end(),
				[&name](const Ref<IPostProcessPass>& pass) {
					return pass->GetName() == name;
				}),
			m_passes.end()
		);
	}

	IPostProcessPass* PostProcessPipeline::GetPass(const std::string& name)
	{
		for (auto& pass : m_passes)
		{
			if (pass->GetName() == name)
				return pass.get();
		}
		return nullptr;
	}

	void PostProcessPipeline::Execute(uint32_t sceneTextureID)
	{
		YUICY_PROFILE_FUNCTION();

		if (!m_initialized)
			return;

		// 收集启用的 Pass
		std::vector<IPostProcessPass*> activePasses;
		for (auto& pass : m_passes)
		{
			if (pass->IsEnabled())
				activePasses.push_back(pass.get());
		}

		if (activePasses.empty())
		{
			// 没有启用的 Pass，不做任何后处理
			return;
		}

		uint32_t currentInputTexture = sceneTextureID;
		bool usePing = true;

		for (size_t i = 0; i < activePasses.size(); ++i)
		{
			bool isLastPass = (i == activePasses.size() - 1);

			if (isLastPass)
			{
				// 最后一个 Pass 直接渲染到屏幕（默认 FBO）
				activePasses[i]->Execute(currentInputTexture, nullptr);
			}
			else
			{
				// 中间 Pass 渲染到 Ping-Pong FBO
				// TODO 开启MSAA时，需要针对 MSAA 纹理做 Reslove 处理，否则后续流程无法正常采样
				auto& targetFBO = usePing ? m_pingFBO : m_pongFBO;
				activePasses[i]->Execute(currentInputTexture, targetFBO);

				currentInputTexture = targetFBO->GetColorAttachmentRendererID(0);
				usePing = !usePing;
			}
		}
	}

	void PostProcessPipeline::CreatePingPongFBOs(uint32_t width, uint32_t height)
	{
		FramebufferSpecification spec;
		spec.width = width;
		spec.height = height;
		spec.attachments = { FramebufferTextureFormat::RGBA8 };

		m_pingFBO = Framebuffer::Create(spec);
		m_pongFBO = Framebuffer::Create(spec);
	}

	void PostProcessPipeline::CreateFullscreenQuad()
	{
		float vertices[] = {
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f
		};

		m_quadVAO = VertexArray::Create();
		m_quadVBO = VertexBuffer::Create(vertices, sizeof(vertices));
		m_quadVBO->SetLayout({
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		});
		m_quadVAO->AddVertexBuffer(m_quadVBO);
	}

}
