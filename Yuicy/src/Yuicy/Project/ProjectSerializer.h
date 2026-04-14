#pragma once

#include "Yuicy/Project/Project.h"

#include <filesystem>
#include <string_view>

namespace Yuicy {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(const Ref<Project>& project);

		void Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

	public:
		inline static std::string_view FileFilter = "Yuicy Project (*.yproj)\0*.yproj\0";
		inline static std::string_view DefaultExtension = ".yproj";

		static const char* GetProjectSerializerFileFilter() { return FileFilter.data(); }
		static const char* GetProjectSerializerDefaultExtension() { return DefaultExtension.data(); }

	private:
		Ref<Project> m_project;
	};

}
