#pragma once

#include "Yuicy/Renderer/RendererAPI.h"

namespace Yuicy {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;
		virtual void SetDepthTest(bool enable) override;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawArrays(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
		virtual void SetLineWidth(float width) override;
		virtual void BindDefaultFramebuffer() override;

	private:
		// Init 中按上下文查询；forward-compatible 上下文（macOS 必须）中为 1，超出会产生 GL_INVALID_VALUE
		float m_MaxLineWidth = 1.0f;
	};


}
