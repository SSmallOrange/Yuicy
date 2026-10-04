#pragma once

#include <filesystem>
#include <optional>

namespace Yuicy {

	struct EditorAutoSaveSettings
	{
		bool enabled = false;
		int intervalSeconds = 300;
	};

	// 按项目、按用户保存的编辑器偏好
	struct EditorProjectUserSettings
	{
		EditorAutoSaveSettings autoSave;
	};

	// 文件位于 <项目目录>/.yuistudio/EditorUserSettings.yaml
	class EditorProjectUserSettingsSerializer
	{
	public:
		static std::filesystem::path GetFilePath(const std::filesystem::path& projectDirectory);

		// 按需创建 .yuistudio 目录，其中没有 .gitignore 时写入一份忽略整个目录的 .gitignore；写入失败时记录错误并返回 false
		static bool Serialize(const EditorProjectUserSettings& settings, const std::filesystem::path& projectDirectory);
		// 文件无法读取或格式错误时记录错误并返回 std::nullopt；缺失的键取默认值
		static std::optional<EditorProjectUserSettings> Deserialize(const std::filesystem::path& projectDirectory);
	};

}
