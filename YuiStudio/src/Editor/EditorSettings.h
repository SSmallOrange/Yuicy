#pragma once

#include <string>
#include <vector>

namespace Yuicy {

	// 编辑器设置
	// 当前阶段只定义最小数据结构和默认值。
	// TODO: 完整的持久化 UI 和读写文件。
	struct EditorSettings
	{
		// Gizmo 默认模式
		// -1 = 无, 对应 ImGuizmo::OPERATION 枚举值
		int defaultGizmoMode = -1;

		// 最近打开记录
		std::vector<std::string> recentProjects;
		std::vector<std::string> recentScenes;

		static constexpr size_t maxRecentEntries = 10;

		// 工具方法
		void AddRecentProject(const std::string& path)
		{
			RemoveFromList(recentProjects, path);
			recentProjects.insert(recentProjects.begin(), path);

			if (recentProjects.size() > maxRecentEntries)
				recentProjects.resize(maxRecentEntries);
		}

		void AddRecentScene(const std::string& path)
		{
			RemoveFromList(recentScenes, path);
			recentScenes.insert(recentScenes.begin(), path);

			if (recentScenes.size() > maxRecentEntries)
				recentScenes.resize(maxRecentEntries);
		}

	private:
		static void RemoveFromList(std::vector<std::string>& list, const std::string& value)
		{
			list.erase(
				std::remove(list.begin(), list.end(), value),
				list.end()
			);
		}
	};

}
