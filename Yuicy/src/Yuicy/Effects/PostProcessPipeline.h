#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/IPostProcessPass.h"
#include "Yuicy/Renderer/Framebuffer.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/Buffer.h"

#include <vector>
#include <string>

namespace Yuicy {

	class PostProcessPipeline
	{
	public:
		PostProcessPipeline() = default;
		~PostProcessPipeline() = default;

		void Init(uint32_t width, uint32_t height);
		void Shutdown();
		void Resize(uint32_t width, uint32_t height);

		void AddPass(Ref<IPostProcessPass> pass);
		void RemovePass(const std::string& name);
		IPostProcessPass* GetPass(const std::string& name);

		// 执行整条 Pass 链
		// sceneTextureID: 场景渲染结果的纹理ID
		// 最终结果渲染到默认FBO（屏幕）
		void Execute(uint32_t sceneTextureID);

		// 全屏四边形 VAO，供各个 Pass 共享
		const Ref<VertexArray>& GetSharedQuadVAO() const { return m_quadVAO; }

	private:
		void CreatePingPongFBOs(uint32_t width, uint32_t height);
		void CreateFullscreenQuad();

	private:
		std::vector<Ref<IPostProcessPass>> m_passes;

		Ref<Framebuffer> m_pingFBO;
		Ref<Framebuffer> m_pongFBO;

		// 共享的全屏四边形
		Ref<VertexArray> m_quadVAO;
		Ref<VertexBuffer> m_quadVBO;

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		bool m_initialized = false;
	};

}
