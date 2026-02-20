#pragma once
#include "OpenGLBasePostProcessPass.h"
#include "Yuicy/Renderer/IVignettePass.h"

namespace Yuicy {

	class OpenGLVignettePass : public OpenGLBasePostProcessPass, public IVignettePass
	{
	public:
		OpenGLVignettePass() { m_name = "Vignette"; }

		void Init() override;
		void Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO) override;

		void SetEnabled(bool enabled) override { OpenGLBasePostProcessPass::SetEnabled(enabled); }
		void SetIntensity(float intensity) override { m_intensity = intensity; }
		void SetRadius(float radius) override { m_radius = radius; }

	private:
		float m_intensity = 0.0f;
		float m_radius = 0.8f;
	};

}
