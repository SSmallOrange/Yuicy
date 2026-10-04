#include "pch.h"

#include "Yuicy/Renderer/SortingLayerConfig.h"

using namespace Yuicy;

TEST_SUITE("Renderer")
{
	TEST_CASE("SortingLayerConfig order lookup and editing")
	{
		SortingLayerConfig config;

		CHECK(config.GetLayerOrder("Background") < config.GetLayerOrder("Default"));
		CHECK(config.GetLayerOrder("Default") < config.GetLayerOrder("Foreground"));
		CHECK(config.GetLayerOrder("Foreground") < config.GetLayerOrder("UI"));

		SUBCASE("unknown layer falls back to order 0")
		{
			CHECK_FALSE(config.HasLayer("Missing"));
			CHECK(config.GetLayerOrder("Missing") == 0);
		}

		SUBCASE("AddLayer ignores duplicate names")
		{
			const size_t count = config.Layers.size();
			config.AddLayer("Default", 999);
			CHECK(config.Layers.size() == count);
			CHECK(config.GetLayerOrder("Default") == 0);
		}

		// SpriteRendererComponent::SortingLayer 的缺省值是 Default，该层必须始终存在
		SUBCASE("RemoveLayer keeps Default")
		{
			config.RemoveLayer("Default");
			CHECK(config.HasLayer("Default"));

			config.RemoveLayer("UI");
			CHECK_FALSE(config.HasLayer("UI"));
		}

		SUBCASE("SortByOrder sorts ascending")
		{
			config.AddLayer("Back", -500);
			config.AddLayer("Top", 500);
			config.SortByOrder();

			CHECK(config.Layers.front().Name == "Back");
			CHECK(config.Layers.back().Name == "Top");
			CHECK(std::ranges::is_sorted(config.Layers, {}, &SortingLayerConfig::Layer::Order));
		}
	}
}
