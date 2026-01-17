#include "pch.h"
#include "OpenGLPostProcessPass.h"

#include <glad/glad.h>

namespace Yuicy {

	OpenGLPostProcessPass::OpenGLPostProcessPass()
	{
	}

	OpenGLPostProcessPass::~OpenGLPostProcessPass()
	{
		Shutdown();
	}

	void OpenGLPostProcessPass::Init()
	{
		YUICY_PROFILE_FUNCTION();

		if (m_initialized)
			return;

		m_shader = Shader::Create("assets/shaders/PostProcess.glsl");
		CreateFullscreenQuad();
		m_initialized = true;

		YUICY_CORE_INFO("OpenGLPostProcessPass: Initialized");
	}

	void OpenGLPostProcessPass::Shutdown()
	{
		YUICY_PROFILE_FUNCTION();

		m_shader = nullptr;
		m_quadVAO = nullptr;
		m_quadVBO = nullptr;
		m_initialized = false;
	}

	void OpenGLPostProcessPass::CreateFullscreenQuad()
	{
		YUICY_PROFILE_FUNCTION();

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

	void OpenGLPostProcessPass::Execute(const Ref<Framebuffer>& sourceFramebuffer, const PostProcessConfig& config)
	{
		YUICY_PROFILE_FUNCTION();

		if (!m_initialized)
		{
			YUICY_CORE_ERROR("OpenGLPostProcessPass: Not initialized!");
			return;
		}

		m_shader->Bind();
		UploadUniforms(config);

		// 绑定源帧缓冲的颜色附件作为纹理
		glBindTextureUnit(0, sourceFramebuffer->GetColorAttachmentRendererID(0));
		m_shader->SetInt("u_ScreenTexture", 0);

		// 绘制全屏四边形
		glDisable(GL_DEPTH_TEST);
		m_quadVAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
	}

	void OpenGLPostProcessPass::UploadUniforms(const PostProcessConfig& config)
	{
		// 颜色调整
		m_shader->SetFloat4("u_AmbientTint", config.ambientTint);
		m_shader->SetFloat("u_Brightness", config.brightness);
		m_shader->SetFloat("u_Contrast", config.contrast);
		m_shader->SetFloat("u_Saturation", config.saturation);

		// 雾效
		m_shader->SetInt("u_FogEnabled", config.fogEnabled ? 1 : 0);
		m_shader->SetFloat4("u_FogColor", config.fogColor);
		m_shader->SetFloat("u_FogDensity", config.fogDensity);

		// 暗角
		m_shader->SetInt("u_VignetteEnabled", config.vignetteEnabled ? 1 : 0);
		m_shader->SetFloat("u_VignetteIntensity", config.vignetteIntensity);
		m_shader->SetFloat("u_VignetteRadius", config.vignetteRadius);

		// 闪光
		m_shader->SetInt("u_FlashEnabled", config.flashEnabled ? 1 : 0);
		m_shader->SetFloat("u_FlashIntensity", config.flashIntensity);
		m_shader->SetFloat3("u_FlashColor", config.flashColor);

		// 屏幕雨滴
		m_shader->SetInt("u_RaindropsEnabled", config.raindropsEnabled ? 1 : 0);
		m_shader->SetFloat("u_RaindropsIntensity", config.raindropsIntensity);
		m_shader->SetFloat("u_RaindropsTime", config.raindropsTime);

		// 2D光照
		m_shader->SetInt("u_LightingEnabled", config.lightingEnabled ? 1 : 0);
		if (config.lightingEnabled && config.lightMapTextureID != 0)
		{
			glBindTextureUnit(1, config.lightMapTextureID);
			m_shader->SetInt("u_LightMap", 1);
		}
	}
}
