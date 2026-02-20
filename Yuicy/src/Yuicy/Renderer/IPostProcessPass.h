#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Framebuffer.h"
#include "Yuicy/Renderer/VertexArray.h"

#include <string>

namespace Yuicy {

	class IPostProcessPass
	{
	public:
		virtual ~IPostProcessPass() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		// inputTextureID: 上一个 Pass 的输出纹理（或场景 FBO 的颜色附件）
		// outputFBO: 当前 Pass 的渲染目标。如果为 nullptr，则渲染到默认 FBO（屏幕）
		virtual void Execute(uint32_t inputTextureID, const Ref<Framebuffer>& outputFBO) = 0;

		virtual bool IsEnabled() const = 0;
		virtual void SetEnabled(bool enabled) = 0;

		virtual const std::string& GetName() const = 0;

		virtual void SetSharedQuadVAO(const Ref<VertexArray>& vao) = 0;
	};

}
