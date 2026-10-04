#include "YuicyTest/SceneFixture.h"

#include "Yuicy/Scene/SceneSerializer.h"

#include <doctest/doctest.h>

namespace Yuicy::Test {

	SceneFixture::SceneFixture()
		: m_Scene(CreateRef<Scene>())
	{
	}

	Ref<Scene> SceneFixture::SerializeRoundTrip(const Ref<Scene>& scene) const
	{
		const std::filesystem::path scenePath = m_TempDirectory.GetPath() / "RoundTrip.yui";
		SceneSerializer(scene).Serialize(scenePath);

		Ref<Scene> loaded = CreateRef<Scene>();
		REQUIRE(SceneSerializer(loaded).Deserialize(scenePath));
		return loaded;
	}

}
