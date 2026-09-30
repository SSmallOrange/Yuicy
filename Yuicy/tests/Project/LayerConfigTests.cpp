#include "pch.h"

#include "Yuicy/Physics/CollisionLayerConfig.h"
#include "Yuicy/Renderer/SortingLayerConfig.h"

using namespace Yuicy;

TEST_SUITE("Project")
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

	TEST_CASE("CollisionLayerConfig maps names to Box2D category bits")
	{
		CollisionLayerConfig config;

		CHECK(config.GetLayerName(0) == "Default");
		CHECK(config.GetBitFromName("Default") == CollisionLayer::Default);
		CHECK(config.GetBitFromName("Layer 15") == 0x8000);
		CHECK(config.GetBitFromName("Missing") == 0);

		config.LayerNames[4] = "Player";
		CHECK(config.GetBitFromName("Player") == (1 << 4));
		CHECK(config.GetBitFromName("Layer 4") == 0);

		config.ResetToDefaults();
		CHECK(config.GetLayerName(4) == "Layer 4");
	}
}
