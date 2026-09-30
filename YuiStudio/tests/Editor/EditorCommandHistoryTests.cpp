#include "pch.h"

#include "Editor/EditorCommandHistory.h"

using namespace Yuicy;

namespace {

	// mergeID 非空时，与下一条 mergeID 相同的命令合并
	class AddValueCommand : public IEditorCommand
	{
	public:
		AddValueCommand(int& target, int delta, std::string mergeID = {})
			: m_target(target),
			  m_delta(delta),
			  m_mergeID(std::move(mergeID))
		{
		}

		void Execute() override { m_target += m_delta; }
		void Undo() override { m_target -= m_delta; }
		std::string GetName() const override { return "Add Value"; }
		std::string GetCommandID() const override { return m_mergeID; }

		// EditorCommandHistory 合并成功后不会再调用新命令的 Execute，因此这里要自己把新命令的效果应用上
		bool TryMerge(const IEditorCommand& other) override
		{
			if (m_mergeID.empty() || other.GetCommandID() != m_mergeID)
				return false;

			const int otherDelta = static_cast<const AddValueCommand&>(other).m_delta;
			m_target += otherDelta;
			m_delta += otherDelta;
			return true;
		}

	private:
		int& m_target;
		int m_delta;
		std::string m_mergeID;
	};

}

TEST_SUITE("Editor")
{
	TEST_CASE("EditorCommandHistory Execute Undo Redo are symmetric")
	{
		EditorCommandHistory history;
		int value = 0;
		int notifications = 0;
		history.SetOnCommandExecuted([&notifications] { notifications++; });

		history.ExecuteCommandT<AddValueCommand>(value, 1);
		history.ExecuteCommandT<AddValueCommand>(value, 10);
		CHECK(value == 11);
		CHECK(history.GetUndoStackSize() == 2);
		CHECK_FALSE(history.CanRedo());

		history.Undo();
		CHECK(value == 1);
		history.Undo();
		CHECK(value == 0);
		CHECK_FALSE(history.CanUndo());
		CHECK(history.GetRedoStackSize() == 2);

		history.Redo();
		CHECK(value == 1);
		history.Redo();
		CHECK(value == 11);
		CHECK_FALSE(history.CanRedo());

		// Undo / Redo 同样修改场景，回调不能只在 ExecuteCommand 时触发
		CHECK(notifications == 6);

		SUBCASE("Undo and Redo on empty stacks are no-ops")
		{
			history.Redo();
			CHECK(value == 11);

			history.Clear();
			CHECK_FALSE(history.CanUndo());
			history.Undo();
			CHECK(value == 11);
		}
	}

	TEST_CASE("EditorCommandHistory new command clears the redo stack")
	{
		EditorCommandHistory history;
		int value = 0;

		history.ExecuteCommandT<AddValueCommand>(value, 1);
		history.ExecuteCommandT<AddValueCommand>(value, 2);
		history.Undo();
		REQUIRE(history.CanRedo());

		history.ExecuteCommandT<AddValueCommand>(value, 100);
		CHECK_FALSE(history.CanRedo());
		CHECK(value == 101);
	}

	TEST_CASE("EditorCommandHistory drops the oldest commands beyond the stack limit")
	{
		EditorCommandHistory history;
		history.SetMaxStackSize(3);
		int value = 0;

		for (int delta : { 1, 2, 4, 8, 16 })
			history.ExecuteCommandT<AddValueCommand>(value, delta);

		CHECK(history.GetUndoStackSize() == 3);
		while (history.CanUndo())
			history.Undo();

		// 只能撤销最近 3 条（4 + 8 + 16），最早的 1 + 2 已被丢弃
		CHECK(value == 3);
	}

	TEST_CASE("EditorCommandHistory merges consecutive commands through TryMerge")
	{
		EditorCommandHistory history;
		int value = 0;

		history.ExecuteCommandT<AddValueCommand>(value, 1, "drag");
		history.ExecuteCommandT<AddValueCommand>(value, 2, "drag");
		history.ExecuteCommandT<AddValueCommand>(value, 3, "drag");
		CHECK(value == 6);
		CHECK(history.GetUndoStackSize() == 1);

		history.ExecuteCommandT<AddValueCommand>(value, 10, "other");
		CHECK(history.GetUndoStackSize() == 2);

		history.Undo();
		history.Undo();
		CHECK(value == 0);
		CHECK_FALSE(history.CanUndo());
	}

	TEST_CASE("EditorCommandHistory ignores null commands")
	{
		EditorCommandHistory history;
		int notifications = 0;
		history.SetOnCommandExecuted([&notifications] { notifications++; });

		history.ExecuteCommand(nullptr);

		CHECK_FALSE(history.CanUndo());
		CHECK(notifications == 0);
	}
}
