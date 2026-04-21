#pragma once

#include <string>
#include <array>
#include <cstdint>

namespace Yuicy {

	// 碰撞层命名配置（16 位，与 Box2D filter bits 对齐）
	struct CollisionLayerConfig
	{
		static constexpr int MaxLayers = 16;

		std::array<std::string, MaxLayers> LayerNames;

		CollisionLayerConfig()
		{
			ResetToDefaults();
		}

		void ResetToDefaults()
		{
			LayerNames[0] = "Default";
			for (int i = 1; i < MaxLayers; i++)
				LayerNames[i] = "Layer " + std::to_string(i);
		}

		const std::string& GetLayerName(int bit) const
		{
			return LayerNames[bit];
		}

		uint16_t GetBitFromName(const std::string& name) const
		{
			for (int i = 0; i < MaxLayers; i++)
			{
				if (LayerNames[i] == name)
					return 1 << i;
			}
			return 0;
		}
	};

}
