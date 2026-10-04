#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Scene/SceneContext.h"

namespace Yuicy {

	struct ProjectSettings;

	// 用项目配置与资源管理器组装 SceneContext；assetManager 为空时场景不加载资源
	SceneContext MakeSceneContext(const ProjectSettings& settings, const Ref<AssetManagerBase>& assetManager);
}
