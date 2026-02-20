#pragma once

#include "Yuicy/Core/Base.h"

namespace Yuicy {

	class IVignettePass
	{
	public:
		virtual ~IVignettePass() = default;

		virtual void SetEnabled(bool enabled) = 0;
		virtual void SetIntensity(float intensity) = 0;
		virtual void SetRadius(float radius) = 0;

		static Ref<IVignettePass> Create();
	};

}
