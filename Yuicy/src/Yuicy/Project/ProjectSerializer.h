#pragma once

#include "Yuicy/Project/Project.h"
#include "Yuicy/Core/FileDialogs.h"

#include <filesystem>
#include <span>
#include <string_view>

namespace Yuicy {

	class ProjectSerializer
	{
	public:
		ProjectSerializer(const Ref<Project>& project);

		void Serialize(const std::filesystem::path& filepath);
		bool Deserialize(const std::filesystem::path& filepath);

	public:
		inline static const FileDialogFilter FileFilter{ "Yuicy Project", { "yproj" } };
		inline static std::string_view DefaultExtension = ".yproj";

		static std::span<const FileDialogFilter> GetProjectSerializerFileFilter() { return { &FileFilter, 1 }; }
		static const char* GetProjectSerializerDefaultExtension() { return DefaultExtension.data(); }

	private:
		Ref<Project> m_project;
	};

}
