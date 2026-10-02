#pragma once

#include "Yuicy/Core/Assert.h"

#include <string>

namespace Yuicy::Test {

	// 生存期内触发的 YUICY_ASSERT / YUICY_CORE_ASSERT 只计数、不判用例失败，被测代码继续执行，用于验证错误路径。
	// 析构时恢复之前的处理函数（默认会把断言报告为用例失败）。处理函数是全局的，不支持嵌套。
	class ScopedAssertCapture
	{
	public:
		ScopedAssertCapture();
		~ScopedAssertCapture();

		ScopedAssertCapture(const ScopedAssertCapture&) = delete;
		ScopedAssertCapture& operator=(const ScopedAssertCapture&) = delete;

		int GetCount() const;
		// 断言的条件表达式原文，如 "handle != 0"；没有触发过时为空
		const std::string& GetLastExpression() const;

	private:
		AssertFailureHandler m_PreviousHandler = nullptr;
	};

}
