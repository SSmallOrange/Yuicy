#include "pch.h"
#include "Yuicy/Core/Assert.h"

namespace Yuicy {

	static AssertFailureHandler s_AssertFailureHandler = nullptr;

	AssertFailureHandler SetAssertFailureHandler(AssertFailureHandler handler)
	{
		return std::exchange(s_AssertFailureHandler, handler);
	}

	namespace Detail {

		bool HandleAssertFailure(const char* file, int line, const char* expression)
		{
			return s_AssertFailureHandler && s_AssertFailureHandler(file, line, expression);
		}

	}

}
