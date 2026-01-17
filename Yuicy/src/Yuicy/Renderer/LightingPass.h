#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Effects/LightTypes.h"
#include <glm/glm.hpp>
#include <vector>

namespace Yuicy {

	class LightingPass
	{
	public:
		virtual ~LightingPass() = default;

		virtual void Init(uint32_t width, uint32_t height) = 0;
		virtual void Shutdown() = 0;
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		virtual void BeginLightMap(const LightingConfig& config) = 0;
		virtual void EndLightMap() = 0;

		// Render a single light
		virtual void RenderLight(
			const Light2D& light,
			const glm::vec2& cameraPos,
			const glm::vec2& viewportSize,
			const std::vector<glm::vec2>* visibilityPolygon = nullptr
		) = 0;

		virtual uint32_t GetLightMapTextureID() const = 0;
		virtual bool IsInitialized() const = 0;

		static Ref<LightingPass> Create();
	};

}
