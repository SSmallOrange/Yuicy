#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Texture.h"

#include <array>
#include <glm/glm.hpp>

namespace Yuicy {
	class SubTexture2D
	{
	public:
		// 右下角 左上角 纹理坐标
		SubTexture2D(const Ref<Texture2D>& texture, const glm::vec2& min, const glm::vec2& max);

		const Ref<Texture2D>& GetTexture() const { return _Texture; }
		const glm::vec2* GetTexCoords() const { return _TexCoords.data(); }

		// cellSize: 每个格子的像素大小 spriteSize: 精灵占用的格子数量
		// eg: CreateFromCoords(texture, {1, 2}, {32, 32}, {2, 3}) 表示从纹理中获取从(32,64)开始，宽64高96的子纹理
		static Ref<SubTexture2D> CreateFromCoords(const Ref<Texture2D>& texture, const glm::vec2& coords, const glm::vec2& cellSize, const glm::vec2& spriteSize = { 1.0f, 1.0f });


	private:
		Ref<Texture2D> _Texture;

		std::array<glm::vec2, 4> _TexCoords;
	};
}
