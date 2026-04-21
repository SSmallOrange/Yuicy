#pragma once

#include <string>
#include <vector>
#include <ranges>

namespace Yuicy {

	// 参考 Unity 的 Sorting Layer 系统：有序列表
	struct SortingLayerConfig
	{
		struct Layer
		{
			std::string Name;
			int Order;    // 渲染优先级（值越小越先渲染，被后续层覆盖）
		};

		std::vector<Layer> Layers;

		SortingLayerConfig()
		{
			ResetToDefaults();
		}

		void ResetToDefaults()
		{
			Layers.clear();
			Layers.push_back({ "Background", -100 });
			Layers.push_back({ "Default",       0 });
			Layers.push_back({ "Foreground",   100 });
			Layers.push_back({ "UI",           200 });
		}

		int GetLayerOrder(const std::string& layerName) const
		{
			for (const auto& layer : Layers)
			{
				if (layer.Name == layerName)
					return layer.Order;
			}
			return 0;
		}

		bool HasLayer(const std::string& layerName) const
		{
			for (const auto& layer : Layers)
			{
				if (layer.Name == layerName)
					return true;
			}
			return false;
		}

		void AddLayer(const std::string& name, int order)
		{
			if (!HasLayer(name))
				Layers.push_back({ name, order });
		}

		void RemoveLayer(const std::string& name)
		{
			if (name == "Default") return;
			Layers.erase(
				std::remove_if(Layers.begin(), Layers.end(),
					[&name](const Layer& l) { return l.Name == name; }),
				Layers.end()
			);
		}

		// 按 Order 排序
		void SortByOrder()
		{
			std::ranges::sort(Layers, {}, &Layer::Order);
		}
	};

}
