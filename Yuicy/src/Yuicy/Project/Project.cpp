#include "pch.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Asset/EditorAssetManager.h"

namespace Yuicy {

	void Project::SetActive(const Ref<Project>& project)
	{
		s_activeProject = project;

		// 初始化 AssetManager
		s_assetManager = CreateRef<EditorAssetManager>();
	}

}
