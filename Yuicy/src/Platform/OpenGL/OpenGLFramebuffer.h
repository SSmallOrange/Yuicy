#pragma once
#pragma once

#include "Yuicy/Renderer/Framebuffer.h"

namespace Yuicy {

	class OpenGLFramebuffer : public Framebuffer
	{
	public:
		OpenGLFramebuffer(const FramebufferSpecification& spec);
		virtual ~OpenGLFramebuffer();

		// 重新创建 Framebuffer（内部使用）
		void Invalidate();

		virtual void Bind() override;
		virtual void Unbind() override;

		virtual void Resize(uint32_t width, uint32_t height) override;
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) override;
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) override;

		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const override
		{
			YUICY_CORE_ASSERT(index < m_colorAttachments.size(), "Index out of bounds!");
			return m_colorAttachments[index];
		}

		virtual const FramebufferSpecification& GetSpecification() const override
		{
			return m_specification;
		}

	private:
		uint32_t m_rendererID = 0;
		FramebufferSpecification m_specification;

		// 附件存储
		std::vector<FramebufferTextureSpecification> m_colorAttachmentSpecifications;
		FramebufferTextureSpecification m_depthAttachmentSpecification = FramebufferTextureFormat::None;

		std::vector<uint32_t> m_colorAttachments;
		uint32_t m_depthAttachment = 0;
	};

}