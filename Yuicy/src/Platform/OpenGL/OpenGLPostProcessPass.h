#pragma once

#include "Yuicy/Renderer/PostProcessPass.h"
#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/Buffer.h"

namespace Yuicy {

	class OpenGLPostProcessPass : public PostProcessPass
	{
	public:
		OpenGLPostProcessPass();
		virtual ~OpenGLPostProcessPass();

		virtual void Init() override;
		virtual void Shutdown() override;
		virtual void Execute(const Ref<Framebuffer>& sourceFramebuffer, const PostProcessConfig& config) override;
		virtual bool IsInitialized() const override { return m_initialized; }

	private:
		void CreateFullscreenQuad();

		void UploadUniforms(const PostProcessConfig& config);

	private:
		Ref<Shader> m_shader;
		Ref<VertexArray> m_quadVAO;
		Ref<VertexBuffer> m_quadVBO;

		bool m_initialized = false;
	};
}
