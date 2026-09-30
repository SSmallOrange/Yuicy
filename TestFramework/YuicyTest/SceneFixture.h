#pragma once

#include "YuicyTest/ScopedProject.h"
#include "YuicyTest/TempDirectory.h"

#include "Yuicy/Core/Base.h"
#include "Yuicy/Scene/Scene.h"

namespace Yuicy::Test {

	// 场景类用例的公共夹具：临时目录 + 活动项目 + 一个空场景，配合 TEST_CASE_FIXTURE 使用。
	// 成员按声明的逆序析构：先释放场景，再关闭项目（写回资产注册表），最后删除临时目录，不要调换顺序。
	class SceneFixture
	{
	protected:
		SceneFixture();

		// 把场景写入临时目录的 .yui 文件再读回一个新场景；读取失败时终止当前用例
		Ref<Scene> SerializeRoundTrip(const Ref<Scene>& scene) const;

	protected:
		ScopedTempDirectory m_TempDirectory;
		ScopedActiveProject m_Project;
		Ref<Scene> m_Scene;
	};

}
