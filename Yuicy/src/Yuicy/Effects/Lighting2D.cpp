#include "pch.h"
#include "Lighting2D.h"
#include "Yuicy/Renderer/LightingPass.h"

#include <algorithm>
#include <cmath>

namespace Yuicy {

	Lighting2D::Lighting2D()
	{
	}

	Lighting2D::~Lighting2D()
	{
		Shutdown();
	}

	void Lighting2D::Init(uint32_t width, uint32_t height)
	{
		if (m_initialized)
			return;

		m_renderPass = LightingPass::Create();
		m_renderPass->Init(width, height);

		m_initialized = true;
	}

	void Lighting2D::Shutdown()
	{
		if (!m_initialized)
			return;

		if (m_renderPass)
		{
			m_renderPass->Shutdown();
			m_renderPass.reset();
		}

		m_lights.clear();
		m_casters.clear();
		m_initialized = false;
	}

	void Lighting2D::Resize(uint32_t width, uint32_t height)
	{
		if (m_renderPass)
			m_renderPass->Resize(width, height);
	}

	uint32_t Lighting2D::AddLight(const Light2D& light)
	{
		uint32_t id = m_nextLightId++;
		m_lights[id] = light;
		return id;
	}

	void Lighting2D::RemoveLight(uint32_t id)
	{
		m_lights.erase(id);
	}

	Light2D* Lighting2D::GetLight(uint32_t id)
	{
		auto it = m_lights.find(id);
		return (it != m_lights.end()) ? &it->second : nullptr;
	}

	const Light2D* Lighting2D::GetLight(uint32_t id) const
	{
		auto it = m_lights.find(id);
		return (it != m_lights.end()) ? &it->second : nullptr;
	}

	void Lighting2D::ClearLights()
	{
		m_lights.clear();
	}

	uint32_t Lighting2D::AddCaster(const ShadowCaster2D& caster)
	{
		uint32_t id = m_nextCasterId++;
		m_casters[id] = caster;
		return id;
	}

	void Lighting2D::RemoveCaster(uint32_t id)
	{
		m_casters.erase(id);
	}

	ShadowCaster2D* Lighting2D::GetCaster(uint32_t id)
	{
		auto it = m_casters.find(id);
		return (it != m_casters.end()) ? &it->second : nullptr;
	}

	void Lighting2D::ClearCasters()
	{
		m_casters.clear();
	}

	void Lighting2D::OnUpdate(Timestep ts)
	{
		// Reserved for animated lights
	}

	void Lighting2D::RenderLightMap(const glm::vec2& cameraPos, const glm::vec2& viewportSize)
	{
		if (!m_initialized || !m_config.enabled)
			return;

		m_renderPass->BeginLightMap(m_config);

		for (auto& [id, light] : m_lights)
		{
			if (!light.enabled)
				continue;

			std::vector<glm::vec2> visPolygon;
			if (light.castShadows && !m_casters.empty())
			{
				visPolygon = CalculateVisibilityPolygon(
					light.position,
					light.radius,
					cameraPos,
					viewportSize
				);
			}

			m_renderPass->RenderLight(
				light,
				cameraPos,
				viewportSize,
				visPolygon.empty() ? nullptr : &visPolygon
			);
		}

		m_renderPass->EndLightMap();
	}

	uint32_t Lighting2D::GetLightMapTextureID() const
	{
		if (m_renderPass)
			return m_renderPass->GetLightMapTextureID();
		return 0;
	}

	std::vector<glm::vec2> Lighting2D::GetCasterWorldVertices(const ShadowCaster2D& caster) const
	{
		std::vector<glm::vec2> vertices;

		switch (caster.shape)
		{
		case ShadowCasterShape::Box:
		{
			glm::vec2 halfSize = caster.size * 0.5f;
			vertices.push_back(caster.position + glm::vec2(-halfSize.x, -halfSize.y));
			vertices.push_back(caster.position + glm::vec2( halfSize.x, -halfSize.y));
			vertices.push_back(caster.position + glm::vec2( halfSize.x,  halfSize.y));
			vertices.push_back(caster.position + glm::vec2(-halfSize.x,  halfSize.y));
			break;
		}
		case ShadowCasterShape::Circle:
		{
			// Approximate circle with 16 vertices
			const int segments = 16;
			for (int i = 0; i < segments; i++)
			{
				float angle = (float)i / (float)segments * 6.283185f;
				vertices.push_back(caster.position + glm::vec2(
					std::cos(angle) * caster.radius,
					std::sin(angle) * caster.radius
				));
			}
			break;
		}
		case ShadowCasterShape::Polygon:
		{
			for (const auto& v : caster.vertices)
			{
				vertices.push_back(caster.position + v);
			}
			break;
		}
		}

		return vertices;
	}

	bool Lighting2D::RaySegmentIntersect(
		const glm::vec2& rayOrigin,
		const glm::vec2& rayDir,
		const glm::vec2& segA,
		const glm::vec2& segB,
		float& t) const
	{
		glm::vec2 v1 = rayOrigin - segA;
		glm::vec2 v2 = segB - segA;
		glm::vec2 v3 = glm::vec2(-rayDir.y, rayDir.x);

		float dot = glm::dot(v2, v3);
		if (std::abs(dot) < 0.00001f)
			return false;

		float t1 = (v2.x * v1.y - v2.y * v1.x) / dot;
		float t2 = glm::dot(v1, v3) / dot;

		if (t1 >= 0.0f && t2 >= 0.0f && t2 <= 1.0f)
		{
			t = t1;
			return true;
		}

		return false;
	}

	std::vector<glm::vec2> Lighting2D::CalculateVisibilityPolygon(
		const glm::vec2& lightPos,
		float lightRadius,
		const glm::vec2& cameraPos,
		const glm::vec2& viewportSize)
	{
		std::vector<RayHit> hits;

		// Collect all edges from casters
		std::vector<std::pair<glm::vec2, glm::vec2>> edges;

		for (const auto& [id, caster] : m_casters)
		{
			if (!caster.enabled)
				continue;

			// Distance check
			float dist = glm::length(caster.position - lightPos);
			if (dist > lightRadius + caster.size.x + caster.size.y)
				continue;

			auto vertices = GetCasterWorldVertices(caster);
			if (vertices.size() < 2)
				continue;

			for (size_t i = 0; i < vertices.size(); i++)
			{
				size_t next = (i + 1) % vertices.size();
				edges.push_back({ vertices[i], vertices[next] });
			}
		}

		// Add viewport boundary edges
		glm::vec2 halfView = viewportSize * 0.5f;
		glm::vec2 tl = cameraPos + glm::vec2(-halfView.x,  halfView.y);
		glm::vec2 tr = cameraPos + glm::vec2( halfView.x,  halfView.y);
		glm::vec2 br = cameraPos + glm::vec2( halfView.x, -halfView.y);
		glm::vec2 bl = cameraPos + glm::vec2(-halfView.x, -halfView.y);

		edges.push_back({ tl, tr });
		edges.push_back({ tr, br });
		edges.push_back({ br, bl });
		edges.push_back({ bl, tl });

		// Collect unique vertices
		std::vector<glm::vec2> uniqueVerts;
		for (const auto& edge : edges)
		{
			uniqueVerts.push_back(edge.first);
			uniqueVerts.push_back(edge.second);
		}

		// Cast rays to each vertex (with small angle offset)
		for (const auto& vertex : uniqueVerts)
		{
			glm::vec2 dir = vertex - lightPos;
			float baseAngle = std::atan2(dir.y, dir.x);

			// Cast 3 rays per vertex
			for (float offset : { -0.0001f, 0.0f, 0.0001f })
			{
				float angle = baseAngle + offset;
				glm::vec2 rayDir = glm::vec2(std::cos(angle), std::sin(angle));

				// Find closest intersection
				float closestT = lightRadius;
				for (const auto& edge : edges)
				{
					float t;
					if (RaySegmentIntersect(lightPos, rayDir, edge.first, edge.second, t))
					{
						if (t < closestT && t > 0.0f)
							closestT = t;
					}
				}

				RayHit hit;
				hit.point = lightPos + rayDir * closestT;
				hit.distance = closestT;
				hit.angle = angle;
				hits.push_back(hit);
			}
		}

		// Sort by angle
		std::sort(hits.begin(), hits.end(), [](const RayHit& a, const RayHit& b) {
			return a.angle < b.angle;
		});

		// Extract polygon
		std::vector<glm::vec2> polygon;
		polygon.reserve(hits.size());
		for (const auto& hit : hits)
		{
			polygon.push_back(hit.point);
		}

		return polygon;
	}

}
