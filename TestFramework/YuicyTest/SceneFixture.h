#pragma once

#include "YuicyTest/TempDirectory.h"

#include "Yuicy/Core/Base.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy::Test {

	// 场景类用例的公共夹具：临时目录 + 一个使用默认上下文（不加载资源）的空场景，配合 TEST_CASE_FIXTURE 使用
	class SceneFixture
	{
	protected:
		SceneFixture();

		// 把场景写入临时目录的 .yui 文件再读回一个新场景；读取失败时终止当前用例
		Ref<Scene> SerializeRoundTrip(const Ref<Scene>& scene) const;

	protected:
		ScopedTempDirectory m_TempDirectory;
		Ref<Scene> m_Scene;
	};

}
