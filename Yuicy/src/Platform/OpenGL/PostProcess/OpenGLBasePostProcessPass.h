#pragma once

#include "Yuicy/Renderer/IPostProcessPass.h"
#include "Yuicy/Renderer/Shader.h"
#include "Yuicy/Renderer/VertexArray.h"
#include "Yuicy/Renderer/RenderCommand.h"

#include <glad/glad.h>

namespace Yuicy {

	// 所有 OpenGL 后处理 Pass 的公共基类
	// 提供全屏四边形绘制、shader 绑定等通用逻辑
	class OpenGLBasePostProcessPass : public IPostProcessPass
	{
	public:
		virtual ~OpenGLBasePostProcessPass() = default;

		void Shutdown() override
		{
			m_shader = nullptr;
			m_initialized = false;
		}

		bool IsEnabled() const override { return m_enabled; }
		void SetEnabled(bool enabled) override { m_enabled = enabled; }
		const std::string& GetName() const override { return m_name; }

	protected:
		// 子类调用: 加载 shader 并标记初始化完成
		void InitShader(const std::string& shaderPath)
		{
			m_shader = Shader::Create(shaderPath);
			m_initialized = true;
		}

		// 子类调用: 绑定输出 FBO (nullptr = 默认屏幕)
		void BindOutputTarget(const Ref<Framebuffer>& outputFBO)
		{
			if (outputFBO)
				outputFBO->Bind();
			else
				RenderCommand::BindDefaultFramebuffer();
		}

		// 子类调用: 解绑输出 FBO
		void UnbindOutputTarget(const Ref<Framebuffer>& outputFBO)
		{
			if (outputFBO)
				outputFBO->Unbind();
		}

		// 子类调用: 绑定输入纹理到指定纹理单元
		void BindInputTexture(uint32_t textureID, uint32_t slot = 0 , const std::string& uniformName = "u_InputTexture")
		{
			glBindTextureUnit(slot, textureID);
			m_shader->SetInt(uniformName, slot);
		}

		// 子类调用: 绘制全屏四边形
		void DrawQuad()
		{
			// TODO 这里是否需要根据当前深度测试的开启状态来判断如何调整 DepthTest
			RenderCommand::SetDepthTest(false);
			RenderCommand::DrawArrays(m_sharedQuadVAO, 6);
			RenderCommand::SetDepthTest(true);
		}

	protected:
		Ref<Shader> m_shader;
		std::string m_name;
		bool m_enabled = false;
		bool m_initialized = false;

		// 由 PostProcessPipeline 设置的共享 quad VAO
		Ref<VertexArray> m_sharedQuadVAO;

	public:
		void SetSharedQuadVAO(const Ref<VertexArray>& vao) override { m_sharedQuadVAO = vao; }
	};

}
