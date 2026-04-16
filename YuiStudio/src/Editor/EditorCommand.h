#pragma once

#include <string>

namespace Yuicy {

	// 编辑器命令基类
	// 所有可撤销的编辑操作都继承此接口。
	// TODO
	class IEditorCommand
	{
	public:
		virtual ~IEditorCommand() = default;

		// 执行命令
		virtual void Execute() = 0;

		// 撤销命令
		virtual void Undo() = 0;

		// 命令名称（用于 UI 显示和调试）
		virtual std::string GetName() const = 0;

		// 尝试与下一个命令合并（如连续拖拽 Gizmo）
		// 返回 true 表示合并成功，不再单独入栈
		virtual bool TryMerge(const IEditorCommand& other) { return false; }
	};

}
