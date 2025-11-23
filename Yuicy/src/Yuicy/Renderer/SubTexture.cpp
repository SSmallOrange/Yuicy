#include "pch.h"
#include "SubTexture.h"

namespace Yuicy {

	Yuicy::SubTexture2D::SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max)
		: _Texture(texture)
	{
		_TexCoords[0] = { min.x, min.y };
		_TexCoords[1] = { max.x, min.y };
		_TexCoords[2] = { max.x, max.y };
		_TexCoords[3] = { min.x, max.y };
	}

	// 坐标、格子大小、精灵大小
	Ref<SubTexture2D> Yuicy::CreateFromCoords(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize)
	{
		glm::vec2 min = { (coords.x * cellSize.x) / texture->GetWidth(), (coords.y * cellSize.y) / texture->GetHeight() };
		glm::vec2 max = { ((coords.x + spriteSize.x) * cellSize.x) / texture->GetWidth(), ((coords.y + spriteSize.y) * cellSize.y) / texture->GetHeight() };
		return CreateRef<SubTexture2D>(texture, min, max);
	}
}
