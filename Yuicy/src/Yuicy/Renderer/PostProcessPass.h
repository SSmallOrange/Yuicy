#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Framebuffer.h"
#include "Yuicy/Effects/PostProcessTypes.h"

namespace Yuicy {

	class PostProcessPass
	{
	public:
		virtual ~PostProcessPass() = default;

		virtual void Init() = 0;

		virtual void Shutdown() = 0;

		virtual void Execute(const Ref<Framebuffer>& sourceFramebuffer, const PostProcessConfig& config) = 0;

		virtual bool IsInitialized() const = 0;

		static Ref<PostProcessPass> Create();
	};

}