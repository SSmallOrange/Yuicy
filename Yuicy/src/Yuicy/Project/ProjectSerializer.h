#pragma once

#include "Yuicy/Project/ProjectSettings.h"
#include "Yuicy/Core/FileDialogs.h"

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>

namespace Yuicy {

	// .yproj 读写：Project 节存放项目描述，其余每个模块一节（Renderer2D、Physics2D），节名与 ProjectSettings 的成员名一致
	class ProjectSerializer
	{
	public:
		// 写入失败时记录错误并返回 false
		static bool Serialize(const ProjectSettings& settings, const std::filesystem::path& filepath);
		// 文件无法读取、缺少 Project.Name 或内容格式错误时记录错误并返回 std::nullopt；缺失的其他键与节取默认值
		static std::optional<ProjectSettings> Deserialize(const std::filesystem::path& filepath);

	public:
		inline static const FileDialogFilter FileFilter{ "Yuicy Project", { "yproj" } };
		inline static std::string_view DefaultExtension = ".yproj";

		static std::span<const FileDialogFilter> GetProjectSerializerFileFilter() { return { &FileFilter, 1 }; }
		static const char* GetProjectSerializerDefaultExtension() { return DefaultExtension.data(); }
	};

}
