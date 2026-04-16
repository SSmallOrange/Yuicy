#pragma once

#include "Yuicy.h"

#include <array>
#include <cstdint>
#include <filesystem>

namespace Yuicy::EditorIconUtils {

	inline std::filesystem::path ResolveAssetPath(const std::filesystem::path& relativePath)
	{
		const std::array<std::filesystem::path, 3> candidates = {
			relativePath,
			std::filesystem::path("YuiStudio") / relativePath,
			std::filesystem::path("..") / ".." / ".." / "YuiStudio" / relativePath
		};

		for (const auto& candidate : candidates)
		{
			std::error_code ec;
			if (std::filesystem::exists(candidate, ec) && !ec)
				return candidate.lexically_normal();
		}

		return relativePath;
	}

	inline Ref<Texture2D> CreateSolidColorTexture(const std::array<uint8_t, 4>& color)
	{
		Ref<Texture2D> texture = Texture2D::Create(1, 1);
		uint8_t pixel[4] = { color[0], color[1], color[2], color[3] };
		texture->SetData(pixel, sizeof(pixel));
		return texture;
	}

	inline Ref<Texture2D> LoadIconTexture(const std::filesystem::path& relativePath, const std::array<uint8_t, 4>& fallbackColor)
	{
		const std::filesystem::path iconPath = ResolveAssetPath(relativePath);

		std::error_code ec;
		if (std::filesystem::exists(iconPath, ec) && !ec)
			return Texture2D::Create(iconPath.string());

		YUICY_CORE_WARN("[Editor] Failed to load icon '{}', using fallback texture.", relativePath.string());
		return CreateSolidColorTexture(fallbackColor);
	}

}
