#include "pch.h"

#include "Yuicy/Scene/SpriteDrawOrder.h"

using namespace Yuicy;

namespace {

	struct NamedSprite
	{
		std::string Name;
		SpriteRendererComponent Sprite;
	};

	NamedSprite MakeSprite(std::string name, std::string sortingLayer, int sortingOrder)
	{
		NamedSprite namedSprite{ std::move(name), {} };
		namedSprite.Sprite.SortingLayer = std::move(sortingLayer);
		namedSprite.Sprite.SortingOrder = sortingOrder;
		return namedSprite;
	}

	// 返回按绘制顺序排列的名字，先绘制的在前
	std::vector<std::string> SortByDrawOrder(std::vector<NamedSprite> sprites, const SortingLayerConfig& sortingLayers)
	{
		std::ranges::sort(sprites, [&sortingLayers](const NamedSprite& a, const NamedSprite& b) {
			return IsSpriteDrawnBefore(a.Sprite, b.Sprite, sortingLayers);
		});

		std::vector<std::string> names;
		for (const auto& sprite : sprites)
			names.push_back(sprite.Name);
		return names;
	}

}

TEST_SUITE("Scene")
{
	TEST_CASE("Sprites are drawn by sorting layer order then by sorting order")
	{
		// 列表顺序与 Order 相反，能区分"按 Order 排序"与"按列表位置排序"
		SortingLayerConfig sortingLayers;
		sortingLayers.Layers = { { "Front", 50 }, { "Back", -50 } };

		const std::vector<NamedSprite> sprites = {
			MakeSprite("FrontLow", "Front", -10),
			MakeSprite("BackHigh", "Back", 10),
			MakeSprite("Unregistered", "Missing", 0),
			MakeSprite("BackLow", "Back", -10),
			MakeSprite("FrontHigh", "Front", 10),
		};

		const std::vector<std::string> expected = { "BackLow", "BackHigh", "Unregistered", "FrontLow", "FrontHigh" };
		CHECK(SortByDrawOrder(sprites, sortingLayers) == expected);
	}

	// 编辑器未打开项目时场景使用默认上下文，排序不能依赖任何项目配置
	TEST_CASE("Default scene context sorts sprites with the default sorting layers")
	{
		const SceneContext context;
		CHECK(context.AssetManager.expired());

		const std::vector<NamedSprite> sprites = {
			MakeSprite("UI", "UI", -100),
			MakeSprite("Foreground", "Foreground", 0),
			MakeSprite("DefaultHigh", "Default", 5),
			MakeSprite("DefaultLow", "Default", -5),
			MakeSprite("Background", "Background", 100),
		};

		const std::vector<std::string> expected = { "Background", "DefaultLow", "DefaultHigh", "Foreground", "UI" };
		CHECK(SortByDrawOrder(sprites, context.Renderer2D.SortingLayers) == expected);
	}
}
