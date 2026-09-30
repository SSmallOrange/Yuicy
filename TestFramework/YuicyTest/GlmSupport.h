#pragma once

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include <cmath>
#include <sstream>

namespace Yuicy::Test {

	template<glm::length_t L>
	struct ApproxVecMatcher
	{
		glm::vec<L, float> Expected;
		float Epsilon;
	};

	// 逐分量按绝对误差 epsilon 比较浮点向量，用法：CHECK(actual == ApproxVec(expected))；失败时 doctest 会打印两侧的值
	template<glm::length_t L>
	ApproxVecMatcher<L> ApproxVec(const glm::vec<L, float>& expected, float epsilon = 1e-4f)
	{
		return { expected, epsilon };
	}

	template<glm::length_t L>
	bool operator==(const glm::vec<L, float>& actual, const ApproxVecMatcher<L>& matcher)
	{
		for (glm::length_t i = 0; i < L; i++)
		{
			if (std::abs(actual[i] - matcher.Expected[i]) > matcher.Epsilon)
				return false;
		}
		return true;
	}

	template<glm::length_t L>
	bool operator!=(const glm::vec<L, float>& actual, const ApproxVecMatcher<L>& matcher)
	{
		return !(actual == matcher);
	}

	template<glm::length_t L, typename T, glm::qualifier Q>
	std::string ToString(const glm::vec<L, T, Q>& value)
	{
		std::ostringstream stream;
		stream << "(";
		for (glm::length_t i = 0; i < L; i++)
			stream << (i == 0 ? "" : ", ") << value[i];
		stream << ")";
		return stream.str();
	}

}

namespace doctest {

	template<glm::length_t L, typename T, glm::qualifier Q>
	struct StringMaker<glm::vec<L, T, Q>>
	{
		static String convert(const glm::vec<L, T, Q>& value)
		{
			return Yuicy::Test::ToString(value).c_str();
		}
	};

	template<glm::length_t L>
	struct StringMaker<Yuicy::Test::ApproxVecMatcher<L>>
	{
		static String convert(const Yuicy::Test::ApproxVecMatcher<L>& matcher)
		{
			return ("Approx" + Yuicy::Test::ToString(matcher.Expected)).c_str();
		}
	};

}
