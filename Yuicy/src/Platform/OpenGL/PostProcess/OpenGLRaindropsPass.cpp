#include "pch.h"
#include "OpenGLRaindropsPass.h"

namespace Yuicy {

	void OpenGLRaindropsPass::Init()
	{
		InitShader("assets/shaders/postprocess/PP_Raindrops.glsl");
		YUICY_CORE_TRACE("OpenGLRaindropsPass: Initialized");
	}

	void OpenGLRaindropsPass::Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO)
	{
		YUICY_PROFILE_FUNCTION();
		if (!m_initialized || !m_sharedQuadVAO) return;

		BindOutputTarget(outputFBO);

		m_shader->Bind();
		BindInputTexture(inputTextureID);
		m_shader->SetFloat("u_RaindropsIntensity", m_intensity);
		m_shader->SetFloat("u_RaindropsTime", m_time);

		DrawQuad();

		UnbindOutputTarget(outputFBO);
	}

}
