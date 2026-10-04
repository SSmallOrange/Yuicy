#include "pch.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Project/BuiltinAssetLoaders.h"

namespace Yuicy {

	void Project::SetActive(const Ref<Project>& project)
	{
		// 旧资源管理器在析构时才写回注册表，必须先释放，新的资源管理器才能读到写回后的内容
		s_assetManager = nullptr;
		s_activeProject = project;
		if (project)
		{
			s_assetManager = CreateRef<EditorAssetManager>(EditorAssetManagerSpecification{
				project->GetAssetDirectory(), project->GetAssetRegistryPath(), CreateBuiltinAssetLoaders() });
		}
	}

}
