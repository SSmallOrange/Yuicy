#pragma once

#include "Yuicy/Core/Base.h"

#include <cstdint>

namespace Yuicy {

	class ILightingComposePass
	{
	public:
		virtual ~ILightingComposePass() = default;

		virtual void SetEnabled(bool enabled) = 0;
		virtual void SetLightMapTextureID(uint32_t textureID) = 0;

		static Ref<ILightingComposePass> Create();
	};

}
