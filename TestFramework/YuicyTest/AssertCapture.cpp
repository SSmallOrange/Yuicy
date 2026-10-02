#include "YuicyTest/AssertCapture.h"

#include <doctest/doctest.h>

namespace Yuicy::Test {

	namespace {

		// AssertFailureHandler 是函数指针，无法携带对象，只能放在文件内的全局状态里
		bool s_Active = false;
		int s_Count = 0;
		std::string s_LastExpression;

		bool CaptureAssertFailure(const char*, int, const char* expression)
		{
			++s_Count;
			s_LastExpression = expression;
			return true;
		}

	}

	ScopedAssertCapture::ScopedAssertCapture()
	{
		REQUIRE_MESSAGE(!s_Active, "ScopedAssertCapture does not support nesting");

		s_Active = true;
		s_Count = 0;
		s_LastExpression.clear();
		m_PreviousHandler = SetAssertFailureHandler(CaptureAssertFailure);
	}

	ScopedAssertCapture::~ScopedAssertCapture()
	{
		SetAssertFailureHandler(m_PreviousHandler);
		s_Active = false;
	}

	int ScopedAssertCapture::GetCount() const
	{
		return s_Count;
	}

	const std::string& ScopedAssertCapture::GetLastExpression() const
	{
		return s_LastExpression;
	}

}
