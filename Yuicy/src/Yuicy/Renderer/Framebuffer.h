#pragma once

#include "Yuicy/Core/Base.h"
#include <glm/glm.hpp>

namespace Yuicy {
	// 帧缓冲格式
	enum class FramebufferTextureFormat
	{
		None = 0,

		// 颜色格式
		RGBA8,              // 标准 8 位 RGBA
		RGBA16F,            // HDR的RGBA
		RED_INTEGER,        // 整数格式（可以存储实体Id，用来实现点击拾取）

		// 深度/模板格式
		DEPTH24STENCIL8,    // 24 位深度 + 8 位模板

		// 默认深度格式
		Depth = DEPTH24STENCIL8
	};

	struct FramebufferTextureSpecification
	{
		FramebufferTextureFormat textureFormat = FramebufferTextureFormat::None;

		FramebufferTextureSpecification() = default;
		FramebufferTextureSpecification(FramebufferTextureFormat format)
			: textureFormat(format) {
		}
	};

	// 附件
	struct FramebufferAttachmentSpecification
	{
		std::vector<FramebufferTextureSpecification> attachments;

		FramebufferAttachmentSpecification() = default;
		FramebufferAttachmentSpecification(std::initializer_list<FramebufferTextureSpecification> attachmentList)
			: attachments(attachmentList) {
		}
	};

	struct FramebufferSpecification
	{
		uint32_t width = 1280;
		uint32_t height = 720;
		uint32_t samples = 1;               // MSAA 采样数（1 = 无抗锯齿）

		FramebufferAttachmentSpecification attachments;

		bool swapChainTarget = false;       // 是否直接渲染到屏幕（交换链）
	};

	// 帧缓冲
	class Framebuffer
	{
	public:
		virtual ~Framebuffer() = default;

		virtual void Bind() = 0;
		virtual void Unbind() = 0;

		// 调整大小（窗口 resize 时调用）
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		// 读取像素值（用于实体拾取等）
		virtual int ReadPixel(uint32_t attachmentIndex, int x, int y) = 0;

		// 清除指定附件
		virtual void ClearAttachment(uint32_t attachmentIndex, int value) = 0;

		// 获取颜色附件的纹理 ID（用于后处理 Shader 采样）
		virtual uint32_t GetColorAttachmentRendererID(uint32_t index = 0) const = 0;

		// 获取规格
		virtual const FramebufferSpecification& GetSpecification() const = 0;

		static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
	};

}