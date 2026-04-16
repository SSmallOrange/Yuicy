#pragma once

#include "EditorContext.h"

namespace Yuicy {

	// 编辑器脏状态追踪器
	// TODO: 完善实现
	class EditorDirtyTracker
	{
	public:
		EditorDirtyTracker() = default;
		~EditorDirtyTracker() = default;

		// 设置所操作的上下文
		void SetContext(EditorContext* context) { m_context = context; }

		// 标记场景为"已修改"
		void MarkSceneDirty();

		// 标记项目为"已修改"
		void MarkProjectDirty();

		// 场景保存后清除脏标记
		void ClearSceneDirty();

		// 项目保存后清除脏标记
		void ClearProjectDirty();

		// 当前场景是否为脏状态
		bool IsSceneDirty() const;

		// 当前项目是否为脏状态
		bool IsProjectDirty() const;

		// TODO 每帧调用，用于检查自动保存时机
		void OnUpdate(float deltaTime);

	private:
		EditorContext* m_context = nullptr;
	};

}
