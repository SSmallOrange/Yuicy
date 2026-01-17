#include "pch.h"
#include "OpenGLLightingPass.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yuicy {

	OpenGLLightingPass::OpenGLLightingPass()
	{
	}

	OpenGLLightingPass::~OpenGLLightingPass()
	{
		Shutdown();
	}

	void OpenGLLightingPass::Init(uint32_t width, uint32_t height)
	{
		if (m_initialized)
			return;

		// 灯光FBO
		FramebufferSpecification lightMapSpec;
		lightMapSpec.width = width;
		lightMapSpec.height = height;
		lightMapSpec.attachments = { FramebufferTextureFormat::RGBA8 };
		m_lightMapFBO = Framebuffer::Create(lightMapSpec);

		// 阴影FBO
		FramebufferSpecification shadowMapSpec;
		shadowMapSpec.width = width;
		shadowMapSpec.height = height;
		shadowMapSpec.attachments = { FramebufferTextureFormat::RGBA8 };
		m_shadowMapFBO = Framebuffer::Create(shadowMapSpec);

		m_lightShader = Shader::Create("assets/shaders/Light2D.glsl");
		m_shadowShader = Shader::Create("assets/shaders/Shadow2D.glsl");

		CreateFullscreenQuad();

		// Create dynamic polygon VAO/VBO
		m_polygonVAO = VertexArray::Create();
		m_polygonVBO = VertexBuffer::Create(MaxPolygonVertices * sizeof(float) * 2);
		m_polygonVBO->SetLayout({
			{ ShaderDataType::Float2, "a_Position" }
		});
		m_polygonVAO->AddVertexBuffer(m_polygonVBO);

		m_initialized = true;
	}

	void OpenGLLightingPass::Shutdown()
	{
		if (!m_initialized)
			return;

		m_lightMapFBO.reset();
		m_shadowMapFBO.reset();
		m_lightShader.reset();
		m_shadowShader.reset();
		m_quadVAO.reset();
		m_quadVBO.reset();
		m_polygonVAO.reset();
		m_polygonVBO.reset();

		m_initialized = false;
	}

	void OpenGLLightingPass::Resize(uint32_t width, uint32_t height)
	{
		if (m_lightMapFBO)
			m_lightMapFBO->Resize(width, height);
		if (m_shadowMapFBO)
			m_shadowMapFBO->Resize(width, height);
	}

	void OpenGLLightingPass::BeginLightMap(const LightingConfig& config)
	{
		m_lightMapFBO->Bind();

		// 创建环境色
		glm::vec3 ambient = config.ambientColor * config.ambientIntensity;
		glClearColor(ambient.r, ambient.g, ambient.b, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Enable additive blending for lights
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);  // 光照叠加 （src * 1 + dst * 1）
	}

	void OpenGLLightingPass::EndLightMap()
	{
		// Restore standard alpha blending (instead of additive)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		m_lightMapFBO->Unbind();
	}

	void OpenGLLightingPass::RenderLight(
		const Light2D& light,
		const glm::vec2& cameraPos,
		const glm::vec2& viewportSize,
		const std::vector<glm::vec2>* visibilityPolygon)
	{
		if (!light.enabled)
			return;

		const auto& spec = m_lightMapFBO->GetSpecification();
		float aspectRatio = static_cast<float>(spec.width) / static_cast<float>(spec.height);

		// 映射屏幕坐标至 [0, 1]
		glm::vec2 screenPos;
		screenPos.x = (light.position.x - cameraPos.x) / viewportSize.x + 0.5f;
		screenPos.y = (light.position.y - cameraPos.y) / viewportSize.y + 0.5f;

		// 映射角度
		float screenRadius = light.radius / viewportSize.y;

		// 渲染阴影Mask
		if (visibilityPolygon && !visibilityPolygon->empty() && light.castShadows)
		{
			// Render visibility polygon to shadow map
			m_shadowMapFBO->Bind();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			RenderVisibilityPolygon(*visibilityPolygon, light.position, cameraPos, viewportSize);

			m_shadowMapFBO->Unbind();
			m_lightMapFBO->Bind();
		}

		// 渲染参数
		m_lightShader->Bind();
		m_lightShader->SetFloat2("u_LightPosition", screenPos);
		m_lightShader->SetFloat3("u_LightColor", light.color);
		m_lightShader->SetFloat("u_LightRadius", screenRadius);
		m_lightShader->SetFloat("u_LightIntensity", light.intensity);
		m_lightShader->SetFloat("u_LightFalloff", light.falloff);
		m_lightShader->SetFloat("u_AspectRatio", aspectRatio);

		// Spot light parameters
		int isSpot = (light.type == Light2DType::Spot) ? 1 : 0;
		m_lightShader->SetInt("u_IsSpotLight", isSpot);
		m_lightShader->SetFloat("u_LightDirection", light.direction);
		m_lightShader->SetFloat("u_InnerAngle", light.innerAngle);
		m_lightShader->SetFloat("u_OuterAngle", light.outerAngle);

		// Shadow map
		int useShadow = (visibilityPolygon && !visibilityPolygon->empty() && light.castShadows) ? 1 : 0;
		m_lightShader->SetInt("u_UseShadowMap", useShadow);
		if (useShadow)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_shadowMapFBO->GetColorAttachmentRendererID());
			m_lightShader->SetInt("u_ShadowMap", 0);
		}

		m_quadVAO->Bind();
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	uint32_t OpenGLLightingPass::GetLightMapTextureID() const
	{
		if (m_lightMapFBO)
			return m_lightMapFBO->GetColorAttachmentRendererID();
		return 0;
	}

	void OpenGLLightingPass::CreateFullscreenQuad()
	{
		float quadVertices[] = {
			// Position    // TexCoord
			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f, -1.0f,  1.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,

			-1.0f, -1.0f,  0.0f, 0.0f,
			 1.0f,  1.0f,  1.0f, 1.0f,
			-1.0f,  1.0f,  0.0f, 1.0f
		};

		m_quadVAO = VertexArray::Create();
		m_quadVBO = VertexBuffer::Create(quadVertices, sizeof(quadVertices));
		m_quadVBO->SetLayout({
			{ ShaderDataType::Float2, "a_Position" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		});
		m_quadVAO->AddVertexBuffer(m_quadVBO);
	}

	void OpenGLLightingPass::RenderVisibilityPolygon(
		const std::vector<glm::vec2>& polygon,
		const glm::vec2& lightPos,
		const glm::vec2& cameraPos,
		const glm::vec2& viewportSize)
	{
		if (polygon.size() < 3)
			return;

		// Build triangle fan vertices (center + perimeter)
		std::vector<float> vertices;
		vertices.reserve((polygon.size() + 1) * 2);

		// Center vertex (light position)
		vertices.push_back(lightPos.x);
		vertices.push_back(lightPos.y);

		// Perimeter vertices
		for (const auto& v : polygon)
		{
			vertices.push_back(v.x);
			vertices.push_back(v.y);
		}

		// Upload to VBO
		m_polygonVBO->SetData(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)));

		// Render with shadow shader
		m_shadowShader->Bind();
		m_shadowShader->SetFloat2("u_CameraPos", cameraPos);
		m_shadowShader->SetFloat2("u_ViewportSize", viewportSize);

		m_polygonVAO->Bind();
		glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(polygon.size() + 1));
	}

}
