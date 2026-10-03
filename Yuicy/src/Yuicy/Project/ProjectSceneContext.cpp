#include "pch.h"
#include "Yuicy/Project/ProjectSceneContext.h"

#include "Yuicy/Project/Project.h"

namespace Yuicy {

	SceneContext MakeSceneContext(const ProjectConfig& config, const Ref<AssetManagerBase>& assetManager)
	{
		SceneContext context;
		context.AssetManager = assetManager;
		context.Renderer2D.SortingLayers = config.SortingLayers;
		return context;
	}

}
