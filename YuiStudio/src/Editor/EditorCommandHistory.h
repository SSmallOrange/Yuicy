#pragma once

#include "EditorCommand.h"
#include "Yuicy/Core/Base.h"

#include <vector>
#include <string>

namespace Yuicy {

	// 编辑器命令历史（Undo/Redo 栈）
	// TODO: 完善实现
	class EditorCommandHistory
	{
	public:
		EditorCommandHistory() = default;
		~EditorCommandHistory() = default;

		// 执行命令并压入 Undo 栈
		void ExecuteCommand(Scope<IEditorCommand> command);

		// 撤销
		void Undo();

		// 重做
		void Redo();

		// 清空所有历史
		void Clear();

		// 是否可以撤销
		bool CanUndo() const;

		// 是否可以重做
		bool CanRedo() const;

		// 获取当前 Undo 栈深度
		size_t GetUndoStackSize() const { return m_undoStack.size(); }

		// 获取当前 Redo 栈深度
		size_t GetRedoStackSize() const { return m_redoStack.size(); }

	private:
		std::vector<Scope<IEditorCommand>> m_undoStack;
		std::vector<Scope<IEditorCommand>> m_redoStack;
	};

}
