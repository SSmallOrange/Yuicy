#pragma once
#include "OpenGLBasePostProcessPass.h"
#include "Yuicy/Renderer/ILightingComposePass.h"

namespace Yuicy {

	class OpenGLLightingComposePass : public OpenGLBasePostProcessPass, public ILightingComposePass
	{
	public:
		OpenGLLightingComposePass() { m_name = "LightingCompose"; }

		void Init() override;
		void Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO) override;

		void SetEnabled(bool enabled) override { OpenGLBasePostProcessPass::SetEnabled(enabled); }
		void SetLightMapTextureID(uint32_t textureID) override { m_lightMapTextureID = textureID; }

	private:
		uint32_t m_lightMapTextureID = 0;
	};

}
