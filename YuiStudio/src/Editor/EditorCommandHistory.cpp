#include "Yuicy/Core/Log.h"
#include "EditorCommandHistory.h"

namespace Yuicy {

	void EditorCommandHistory::ExecuteCommand(Scope<IEditorCommand> command)
	{
		if (!command)
			return;
		YUICY_CORE_INFO("Command Name: {}", command->GetCommandID());
		// 尝试与栈顶命令合并
		if (!m_undoStack.empty() && m_undoStack.back()->TryMerge(*command))
		{
			NotifyCommandExecuted();
			return;
		}

		command->Execute();
		m_undoStack.push_back(std::move(command));

		// 执行新命令后清空 Redo 栈
		m_redoStack.clear();

		EnforceStackLimit();
		NotifyCommandExecuted();
	}

	void EditorCommandHistory::Undo()
	{
		if (!CanUndo())
			return;

		auto& command = m_undoStack.back();
		command->Undo();
		m_redoStack.push_back(std::move(command));
		m_undoStack.pop_back();

		NotifyCommandExecuted();
	}

	void EditorCommandHistory::Redo()
	{
		if (!CanRedo())
			return;

		auto& command = m_redoStack.back();
		command->Execute();
		m_undoStack.push_back(std::move(command));
		m_redoStack.pop_back();

		NotifyCommandExecuted();
	}

	void EditorCommandHistory::Clear()
	{
		m_undoStack.clear();
		m_redoStack.clear();
	}

	bool EditorCommandHistory::CanUndo() const
	{
		return !m_undoStack.empty();
	}

	bool EditorCommandHistory::CanRedo() const
	{
		return !m_redoStack.empty();
	}

	void EditorCommandHistory::EnforceStackLimit()
	{
		while (m_undoStack.size() > m_maxStackSize)
			m_undoStack.erase(m_undoStack.begin());
	}

	void EditorCommandHistory::NotifyCommandExecuted()
	{
		if (m_onCommandExecuted)
			m_onCommandExecuted();
	}

}
