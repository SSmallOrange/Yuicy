#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Core/Timestep.h"
#include "Yuicy/Effects/LightTypes.h"
#include "Yuicy/Renderer/Framebuffer.h"

#include <vector>
#include <unordered_map>

namespace Yuicy {

	class LightingPass;

	class Lighting2D
	{
	public:
		Lighting2D();
		~Lighting2D();

		void Init(uint32_t width, uint32_t height);
		void Shutdown();
		void Resize(uint32_t width, uint32_t height);

	public:
		// 灯光
		uint32_t AddLight(const Light2D& light);
		void RemoveLight(uint32_t id);
		Light2D* GetLight(uint32_t id);
		const Light2D* GetLight(uint32_t id) const;
		void ClearLights();

		// 阴影
		uint32_t AddCaster(const ShadowCaster2D& caster);
		void RemoveCaster(uint32_t id);
		ShadowCaster2D* GetCaster(uint32_t id);
		void ClearCasters();

	public:
		void OnUpdate(Timestep ts);

		void RenderLightMap(const glm::vec2& cameraPos, const glm::vec2& viewportSize);
		uint32_t GetLightMapTextureID() const;

	public:
		void SetConfig(const LightingConfig& config) { m_config = config; }
		LightingConfig& GetConfig() { return m_config; }
		const LightingConfig& GetConfig() const { return m_config; }

		bool IsEnabled() const { return m_config.enabled; }
		void SetEnabled(bool enabled) { m_config.enabled = enabled; }

	private:
		// Calculate visibility polygon for shadow casting
		std::vector<glm::vec2> CalculateVisibilityPolygon(const glm::vec2& lightPos, float lightRadius, const glm::vec2& cameraPos, const glm::vec2& viewportSize);

		// Get world vertices from shadow caster
		std::vector<glm::vec2> GetCasterWorldVertices(const ShadowCaster2D& caster) const;

		// Ray-segment intersection
		struct RayHit
		{
			glm::vec2 point;
			float distance;
			float angle;
		};
		bool RaySegmentIntersect(const glm::vec2& rayOrigin, const glm::vec2& rayDir, 
			const glm::vec2& segA, const glm::vec2& segB, float& t) const;

	private:
		LightingConfig m_config;

		std::unordered_map<uint32_t, Light2D> m_lights;
		std::unordered_map<uint32_t, ShadowCaster2D> m_casters;
		uint32_t m_nextLightId = 1;
		uint32_t m_nextCasterId = 1;

		Ref<LightingPass> m_renderPass;
		bool m_initialized = false;
	};

}
