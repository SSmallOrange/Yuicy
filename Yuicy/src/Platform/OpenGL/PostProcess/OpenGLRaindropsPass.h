#pragma once
#include "OpenGLBasePostProcessPass.h"
#include "Yuicy/Renderer/IRaindropsPass.h"

namespace Yuicy {

	class OpenGLRaindropsPass : public OpenGLBasePostProcessPass, public IRaindropsPass
	{
	public:
		OpenGLRaindropsPass() { m_name = "Raindrops"; }

		void Init() override;
		void Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO) override;

		void SetEnabled(bool enabled) override { OpenGLBasePostProcessPass::SetEnabled(enabled); }
		void SetIntensity(float intensity) override { m_intensity = intensity; }
		void SetTime(float time) override { m_time = time; }

	private:
		float m_intensity = 0.5f;
		float m_time = 0.0f;
	};

}
