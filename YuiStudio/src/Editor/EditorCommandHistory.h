#pragma once

#include "EditorCommand.h"
#include "Yuicy/Core/Base.h"

#include <vector>
#include <functional>

namespace Yuicy {

	// 编辑器命令历史（Undo/Redo 栈）
	class EditorCommandHistory
	{
	public:
		static constexpr size_t kDefaultMaxStackSize = 100;

		EditorCommandHistory() = default;
		~EditorCommandHistory() = default;

		// 执行命令并压入 Undo 栈
		void ExecuteCommand(Scope<IEditorCommand> command);

		template<typename T, typename... Arg>
		void ExecuteCommandT(Arg&&... args)
		{
			ExecuteCommand(CreateScope<T>(std::forward<Arg>(args)...));
		}

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

		// 命令执行/撤销/重做后的回调
		using OnCommandCallback = std::function<void()>;
		void SetOnCommandExecuted(OnCommandCallback callback) { m_onCommandExecuted = std::move(callback); }

		// 栈容量上限
		void SetMaxStackSize(size_t maxSize) { m_maxStackSize = maxSize; }

	private:
		void EnforceStackLimit();
		void NotifyCommandExecuted();

		std::vector<Scope<IEditorCommand>> m_undoStack;
		std::vector<Scope<IEditorCommand>> m_redoStack;

		OnCommandCallback m_onCommandExecuted;
		size_t m_maxStackSize = kDefaultMaxStackSize;
	};

}
