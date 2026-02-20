#include "pch.h"
#include "OpenGLLightingComposePass.h"

namespace Yuicy {

	void OpenGLLightingComposePass::Init()
	{
		InitShader("assets/shaders/postprocess/PP_LightingCompose.glsl");
		YUICY_CORE_TRACE("OpenGLLightingComposePass: Initialized");
	}

	void OpenGLLightingComposePass::Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO)
	{
		YUICY_PROFILE_FUNCTION();
		if (!m_initialized || !m_sharedQuadVAO) return;

		BindOutputTarget(outputFBO);

		m_shader->Bind();
		BindInputTexture(inputTextureID, 0, "u_InputTexture");

		if (m_lightMapTextureID != 0)
		{
			BindInputTexture(m_lightMapTextureID, 1, "u_LightMap");
		}

		DrawQuad();

		UnbindOutputTarget(outputFBO);
	}

}
