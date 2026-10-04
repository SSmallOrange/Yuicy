#include "pch.h"

#include "Yuicy/Physics/CollisionLayerConfig.h"

using namespace Yuicy;

TEST_SUITE("Physics")
{
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
