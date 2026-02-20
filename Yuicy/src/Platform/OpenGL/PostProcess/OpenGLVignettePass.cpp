#include "pch.h"
#include "OpenGLVignettePass.h"

namespace Yuicy {

	void OpenGLVignettePass::Init()
	{
		InitShader("assets/shaders/postprocess/PP_Vignette.glsl");
		YUICY_CORE_TRACE("OpenGLVignettePass: Initialized");
	}

	void OpenGLVignettePass::Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO)
	{
		YUICY_PROFILE_FUNCTION();
		if (!m_initialized || !m_sharedQuadVAO) return;

		BindOutputTarget(outputFBO);

		m_shader->Bind();
		BindInputTexture(inputTextureID);
		m_shader->SetFloat("u_VignetteIntensity", m_intensity);
		m_shader->SetFloat("u_VignetteRadius", m_radius);

		DrawQuad();

		UnbindOutputTarget(outputFBO);
	}

}
