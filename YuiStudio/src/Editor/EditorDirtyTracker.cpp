#include "pch.h"

#include "EditorDirtyTracker.h"
#include "EditorContext.h"
#include "Yuicy/Project/Project.h"

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
		{
			m_context->document.sceneDirty = false;
			m_autoSaveTimer = 0.0f;
		}
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
		if (!m_context || !m_context->runtime.IsEditing())
			return;

		if (!IsSceneDirty())
			return;

		auto project = Project::GetActive();
		if (!project || !project->GetConfig().EnableAutoSave)
			return;

		m_autoSaveTimer += deltaTime;

		float interval = static_cast<float>(project->GetConfig().AutoSaveIntervalSeconds);
		if (m_autoSaveTimer < interval)
			return;

		// 安全时机检查，防止操作中保存
		if (m_isSafeToAutoSave && !m_isSafeToAutoSave())
			return;

		m_autoSaveTimer = 0.0f;
		if (m_autoSaveCallback)
			m_autoSaveCallback();
	}

}
