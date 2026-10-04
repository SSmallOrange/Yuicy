#include "pch.h"
#include "Yuicy/Project/ProjectSceneContext.h"

#include "Yuicy/Project/ProjectSettings.h"

namespace Yuicy {

	SceneContext MakeSceneContext(const ProjectSettings& settings, const Ref<AssetManagerBase>& assetManager)
	{
		SceneContext context;
		context.AssetManager = assetManager;
		context.Renderer2D = settings.Renderer2D;
		return context;
	}

}
