#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace Yuicy {

	// Light source type
	enum class Light2DType : uint8_t
	{
		Point = 0,      // Point light (circular falloff)
		Spot            // Spot light (flashlight cone)
	};

	// 2D Light source configuration
	struct Light2D
	{
		Light2DType type = Light2DType::Point;
		glm::vec2 position = { 0.0f, 0.0f };    // World position
		glm::vec3 color = { 1.0f, 1.0f, 1.0f }; // RGB color
		float intensity = 1.0f;                  // Intensity 0~inf
		float radius = 5.0f;                     // Light radius (world units)
		float falloff = 2.0f;                    // Falloff curve (1=linear, 2=quadratic)

		// Spot light only
		float direction = 0.0f;                  // Direction angle (radians)
		float innerAngle = 0.3f;                 // Inner cone angle (full brightness)
		float outerAngle = 0.6f;                 // Outer cone angle (falloff to 0)

		bool castShadows = false;                // 开启投射阴影
		int shadowRayCount = 90;                 // Shadow ray count (precision)

		bool enabled = true;                     // Light enabled
	};

	// Shadow caster shape
	enum class ShadowCasterShape : uint8_t
	{
		Box = 0,
		Circle,
		Polygon
	};

	// 2D Shadow caster
	struct ShadowCaster2D
	{
		ShadowCasterShape shape = ShadowCasterShape::Box;
		glm::vec2 position = { 0.0f, 0.0f };    // World position (center)
		glm::vec2 size = { 1.0f, 1.0f };        // Box size
		float radius = 0.5f;                     // Circle radius
		std::vector<glm::vec2> vertices;         // Polygon vertices (local coords)

		bool enabled = true;
	};

	// Lighting system configuration
	struct LightingConfig
	{
		bool enabled = false;
		glm::vec3 ambientColor = { 0.1f, 0.1f, 0.15f };  // Ambient light color
		float ambientIntensity = 0.2f;                    // Ambient intensity

		static constexpr int MaxLights = 32;
	};

}
