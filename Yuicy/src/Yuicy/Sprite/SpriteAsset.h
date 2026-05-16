#pragma once

#include "Yuicy/Asset/Asset.h"

#include <vector>

#include <glm/glm.hpp>

namespace Yuicy {

	struct SpriteAsset : public Asset
	{
		AssetHandle m_textureHandle = 0;
		glm::vec2 m_uvMin = { 0.0f, 0.0f };
		glm::vec2 m_uvMax = { 1.0f, 1.0f };
		glm::vec2 m_pivot = { 0.5f, 0.5f };
		glm::vec4 m_border = { 0.0f, 0.0f, 0.0f, 0.0f };
		float m_pixelsPerUnit = 100.0f;
		std::vector<glm::vec2> m_physicsShape;

		static AssetType GetStaticType() { return AssetType::Sprite; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

}
