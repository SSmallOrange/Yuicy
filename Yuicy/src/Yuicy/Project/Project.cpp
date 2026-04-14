#include "pch.h"
#include "Yuicy/Project/Project.h"

namespace Yuicy {

	void Project::SetActive(const Ref<Project>& project)
	{
		s_activeProject = project;
	}

}
