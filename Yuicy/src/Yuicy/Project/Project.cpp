#include "pch.h"
#include "Yuicy/Project/Project.h"
#include "Yuicy/Asset/EditorAssetManager.h"

namespace Yuicy {

	void Project::SetActive(const Ref<Project>& project)
	{
		// 旧 EditorAssetManager 析构时会把注册表写回活动项目的资产目录，必须先于切换 s_activeProject 释放：
		// 否则关闭项目时访问空指针，切换项目时把旧注册表写进新项目
		s_assetManager = nullptr;
		s_activeProject = project;
		if (project)
			s_assetManager = CreateRef<EditorAssetManager>();
	}

}
