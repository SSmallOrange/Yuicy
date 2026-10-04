#include "pch.h"
#include "Yuicy/Project/Project.h"

#include <system_error>

namespace Yuicy {

	static std::filesystem::path MakeAbsoluteProjectFile(const std::filesystem::path& projectFile)
	{
		std::error_code ec;
		std::filesystem::path absoluteFile = std::filesystem::absolute(projectFile, ec);
		if (ec)
		{
			YUICY_CORE_WARN("[Project] Failed to resolve absolute path of '{}': {}", projectFile.string(), ec.message());
			absoluteFile = projectFile;
		}

		return absoluteFile.lexically_normal();
	}

	Project::Project(const std::filesystem::path& projectFile, ProjectSettings settings)
		: m_projectFile(MakeAbsoluteProjectFile(projectFile)),
		  m_settings(std::move(settings))
	{
	}

}
