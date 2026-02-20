#pragma once

#include "Yuicy/Core/Base.h"

namespace Yuicy {

	class IRaindropsPass
	{
	public:
		virtual ~IRaindropsPass() = default;

		virtual void SetEnabled(bool enabled) = 0;
		virtual void SetIntensity(float intensity) = 0;
		virtual void SetTime(float time) = 0;

		static Ref<IRaindropsPass> Create();
	};

}
