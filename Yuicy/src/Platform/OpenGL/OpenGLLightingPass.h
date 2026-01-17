#pragma once

#include "Yuicy/Renderer/LightingPass.h"
#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/Buffer.h"
#include "Yuicy/Renderer/Framebuffer.h"

namespace Yuicy {

	class OpenGLLightingPass : public LightingPass
	{
	public:
		OpenGLLightingPass();
		virtual ~OpenGLLightingPass();

		void Init(uint32_t width, uint32_t height) override;
		void Shutdown() override;
		void Resize(uint32_t width, uint32_t height) override;

		void BeginLightMap(const LightingConfig& config) override;
		void EndLightMap() override;

		void RenderLight(
			const Light2D& light,
			const glm::vec2& cameraPos,
			const glm::vec2& viewportSize,
			const std::vector<glm::vec2>* visibilityPolygon = nullptr
		) override;

		uint32_t GetLightMapTextureID() const override;
		bool IsInitialized() const override { return m_initialized; }

	private:
		void CreateFullscreenQuad();
		void RenderVisibilityPolygon(
			const std::vector<glm::vec2>& polygon,
			const glm::vec2& lightPos,
			const glm::vec2& cameraPos,
			const glm::vec2& viewportSize);

	private:
		Ref<Framebuffer> m_lightMapFBO;
		Ref<Framebuffer> m_shadowMapFBO;

		Ref<Shader> m_lightShader;
		Ref<Shader> m_shadowShader;

		Ref<VertexArray> m_quadVAO;
		Ref<VertexBuffer> m_quadVBO;

		Ref<VertexArray> m_polygonVAO;
		Ref<VertexBuffer> m_polygonVBO;
		static constexpr uint32_t MaxPolygonVertices = 512;

		bool m_initialized = false;
	};

}
