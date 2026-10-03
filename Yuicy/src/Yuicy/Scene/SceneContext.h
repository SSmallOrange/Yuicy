#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Renderer2DSettings.h"

namespace Yuicy {

	class AssetManagerBase;

	// Scene 运行时需要的外部服务与配置，由宿主（编辑器、Sandbox、测试）提供。
	// 默认构造的 SceneContext 也是合法的：使用默认排序层，不加载任何资源。
	struct SceneContext
	{
		// 不延长资源管理器的寿命：资源管理器释放后 Scene 退化为不加载资源
		WeakRef<AssetManagerBase> AssetManager;

		// 按值保存，配置修改后需要宿主重新调用 Scene::SetContext
		Renderer2DSettings Renderer2D;
	};

}
