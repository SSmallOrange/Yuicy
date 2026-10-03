#include "pch.h"
#include "Yuicy/Scene/SpriteDrawOrder.h"

#include "Yuicy/Renderer/SortingLayerConfig.h"
#include "Yuicy/Scene/Components.h"

namespace Yuicy {

	bool IsSpriteDrawnBefore(const SpriteRendererComponent& a, const SpriteRendererComponent& b, const SortingLayerConfig& sortingLayers)
	{
		const int layerA = sortingLayers.GetLayerOrder(a.SortingLayer);
		const int layerB = sortingLayers.GetLayerOrder(b.SortingLayer);
		if (layerA != layerB)
			return layerA < layerB;

		return a.SortingOrder < b.SortingOrder;
	}

}
