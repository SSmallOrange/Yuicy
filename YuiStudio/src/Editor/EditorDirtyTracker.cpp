#include "EditorDirtyTracker.h"

namespace Yuicy {

	void EditorDirtyTracker::MarkSceneDirty()
	{
		if (m_context)
			m_context->document.sceneDirty = true;
	}

	void EditorDirtyTracker::MarkProjectDirty()
	{
		if (m_context)
			m_context->document.projectDirty = true;
	}

	void EditorDirtyTracker::ClearSceneDirty()
	{
		if (m_context)
			m_context->document.sceneDirty = false;
	}

	void EditorDirtyTracker::ClearProjectDirty()
	{
		if (m_context)
			m_context->document.projectDirty = false;
	}

	bool EditorDirtyTracker::IsSceneDirty() const
	{
		return m_context && m_context->document.sceneDirty;
	}

	bool EditorDirtyTracker::IsProjectDirty() const
	{
		return m_context && m_context->document.projectDirty;
	}

	void EditorDirtyTracker::OnUpdate(float deltaTime)
	{
		// TODO
		// - 检查 AutoSave 定时器
		// - 根据 ProjectConfig 的 EnableAutoSave / AutoSaveIntervalSeconds
		// - 在安全时机（非 Gizmo 拖拽中）触发自动保存
	}

}
