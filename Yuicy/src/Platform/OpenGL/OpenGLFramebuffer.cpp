#include "pch.h"
#include "OpenGLFramebuffer.h"

#include <glad/glad.h>

namespace Yuicy {

	// 最大支持的 Framebuffer 尺寸
	static const uint32_t s_maxFramebufferSize = 8192;

	// 判断是否为深度格式
	static bool IsDepthFormat(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::DEPTH24STENCIL8:
			return true;
		default:
			return false;
		}
	}

	// 将格式转换为 OpenGL 纹理格式
	static GLenum TextureFormatToGL(FramebufferTextureFormat format)
	{
		switch (format)
		{
		case FramebufferTextureFormat::RGBA8:       return GL_RGBA8;
		case FramebufferTextureFormat::RGBA16F:     return GL_RGBA16F;
		case FramebufferTextureFormat::RED_INTEGER: return GL_R32I;
		default:
			YUICY_CORE_ASSERT(false, "Unknown texture format!");
			return 0;
		}
	}

	// 绑定纹理到 Framebuffer
	static void BindTexture(bool multisampled, uint32_t id)
	{
		glBindTexture(multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, id);
	}

	// 创建纹理
	static void CreateTextures(bool multisampled, uint32_t* outID, uint32_t count)
	{
		glCreateTextures(multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, count, outID);
	}

	// 附加颜色纹理
	static void AttachColorTexture(uint32_t id, int samples, GLenum internalFormat, GLenum format,
		uint32_t width, uint32_t height, int index)
	{
		bool multisampled = samples > 1;

		if (multisampled)
		{
			// 多重采样纹理
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, internalFormat, width, height, GL_FALSE);
		}
		else
		{
			// 普通纹理
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, nullptr);

			// 设置纹理参数
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		// 附加到 Framebuffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index,
			multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, id, 0);
	}

	// 附加深度纹理
	static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum attachmentType,
		uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;

		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType,
			multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D, id, 0);
	}

	OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferSpecification& spec)
		: m_specification(spec)
	{
		// 分离颜色附件和深度附件的规格
		for (auto& attachment : m_specification.attachments.attachments)
		{
			if (IsDepthFormat(attachment.textureFormat))
			{
				m_depthAttachmentSpecification = attachment;  // 只能有一个深度附件
			}
			else
			{
				m_colorAttachmentSpecifications.push_back(attachment);
			}
		}

		// 创建 Framebuffer
		Invalidate();
	}

	OpenGLFramebuffer::~OpenGLFramebuffer()
	{
		glDeleteFramebuffers(1, &m_rendererID);
		glDeleteTextures((GLsizei)m_colorAttachments.size(), m_colorAttachments.data());
		glDeleteTextures(1, &m_depthAttachment);
	}

	void OpenGLFramebuffer::Invalidate()
	{
		// 如果已存在，先删除旧资源
		if (m_rendererID)
		{
			glDeleteFramebuffers(1, &m_rendererID);
			glDeleteTextures((GLsizei)m_colorAttachments.size(), m_colorAttachments.data());
			glDeleteTextures(1, &m_depthAttachment);

			m_colorAttachments.clear();
			m_depthAttachment = 0;
		}

		// 创建 Framebuffer 对象
		glCreateFramebuffers(1, &m_rendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);

		bool multisample = m_specification.samples > 1;

		// 创建颜色附件
		if (!m_colorAttachmentSpecifications.empty())
		{
			m_colorAttachments.resize(m_colorAttachmentSpecifications.size());
			CreateTextures(multisample, m_colorAttachments.data(), (uint32_t)m_colorAttachments.size());

			for (size_t i = 0; i < m_colorAttachments.size(); i++)
			{
				BindTexture(multisample, m_colorAttachments[i]);

				switch (m_colorAttachmentSpecifications[i].textureFormat)
				{
				case FramebufferTextureFormat::RGBA8:
					AttachColorTexture(m_colorAttachments[i], m_specification.samples,
						GL_RGBA8, GL_RGBA, m_specification.width, m_specification.height, (int)i);
					break;

				case FramebufferTextureFormat::RGBA16F:
					AttachColorTexture(m_colorAttachments[i], m_specification.samples,
						GL_RGBA16F, GL_RGBA, m_specification.width, m_specification.height, (int)i);
					break;

				case FramebufferTextureFormat::RED_INTEGER:
					AttachColorTexture(m_colorAttachments[i], m_specification.samples,
						GL_R32I, GL_RED_INTEGER, m_specification.width, m_specification.height, (int)i);
					break;

				default:
					break;
				}
			}
		}

		// 创建深度附件
		if (m_depthAttachmentSpecification.textureFormat != FramebufferTextureFormat::None)
		{
			CreateTextures(multisample, &m_depthAttachment, 1);
			BindTexture(multisample, m_depthAttachment);

			switch (m_depthAttachmentSpecification.textureFormat)
			{
			case FramebufferTextureFormat::DEPTH24STENCIL8:
				AttachDepthTexture(m_depthAttachment, m_specification.samples,
					GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT,
					m_specification.width, m_specification.height);
				break;

			default:
				break;
			}
		}

		// 设置多个颜色缓冲的绘制目标
		if (m_colorAttachments.size() > 1)
		{
			YUICY_CORE_ASSERT(m_colorAttachments.size() <= 4, "Only support up to 4 color attachments!");

			GLenum buffers[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
								  GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers((GLsizei)m_colorAttachments.size(), buffers);
		}
		else if (m_colorAttachments.empty())
		{
			// 仅深度 pass（无颜色输出）
			glDrawBuffer(GL_NONE);
		}

		YUICY_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
			"Framebuffer is incomplete!");

		// 解绑
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFramebuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_rendererID);
		glViewport(0, 0, m_specification.width, m_specification.height);
	}

	void OpenGLFramebuffer::Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);  // 绑定回默认缓冲
	}

	void OpenGLFramebuffer::Resize(uint32_t width, uint32_t height)
	{
		// 防止无效尺寸
		if (width == 0 || height == 0 || width > s_maxFramebufferSize || height > s_maxFramebufferSize)
		{
			YUICY_CORE_WARN("Attempted to resize framebuffer to ({}, {})", width, height);
			return;
		}

		m_specification.width = width;
		m_specification.height = height;

		// 重新创建
		Invalidate();
	}

	int OpenGLFramebuffer::ReadPixel(uint32_t attachmentIndex, int x, int y)
	{
		YUICY_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(), "Attachment index out of bounds!");

		glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

		int pixelData;
		glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

		return pixelData;
	}

	void OpenGLFramebuffer::ClearAttachment(uint32_t attachmentIndex, int value)
	{
		YUICY_CORE_ASSERT(attachmentIndex < m_colorAttachments.size(), "Attachment index out of bounds!");

		auto& spec = m_colorAttachmentSpecifications[attachmentIndex];

		glClearTexImage(m_colorAttachments[attachmentIndex], 0, GL_RED_INTEGER, GL_INT, &value);
	}

}