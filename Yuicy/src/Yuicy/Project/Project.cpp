#include "pch.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Asset/EditorAssetManager.h"

namespace Yuicy {

	void Project::SetActive(const Ref<Project>& project)
	{
		s_activeProject = project;
		s_assetManager = project ? CreateRef<EditorAssetManager>() : nullptr;
	}

}
