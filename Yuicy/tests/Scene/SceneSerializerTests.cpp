#include "pch.h"

#include "Yuicy/Asset/RuntimeAssetManager.h"
#include "Yuicy/Project/BuiltinAssetLoaders.h"
#include "Yuicy/Scene/SceneSerializer.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

namespace {

	// 所有字段都取非默认值：取默认值时，反序列化漏读某个字段也能蒙混过关
	void AddAllSerializableComponents(Entity entity)
	{
		auto& transform = entity.GetComponent<TransformComponent>();
		transform.Translation = { 1.5f, -2.25f, 3.0f };
		transform.Rotation = { 0.1f, 0.2f, 0.3f };
		transform.Scale = { 2.0f, 3.0f, 4.0f };

		auto& sprite = entity.AddComponent<SpriteRendererComponent>();
		sprite.Color = { 0.1f, 0.2f, 0.3f, 0.4f };
		sprite.TextureHandle = 12345;
		sprite.TilingFactor = 2.5f;
		sprite.FlipX = true;
		sprite.FlipY = true;
		sprite.SortingLayer = "Foreground";
		sprite.SortingOrder = 7;

		auto& camera = entity.AddComponent<CameraComponent>();
		camera.Camera.SetOrthographicSize(15.0f);
		camera.Camera.SetOrthographicNearClip(-5.0f);
		camera.Camera.SetOrthographicFarClip(5.0f);
		camera.Primary = false;
		camera.FixedAspectRatio = true;
		camera.DesignWidth = 1280;
		camera.DesignHeight = 720;
		camera.ShowSafeArea = true;
		camera.SafeAreaMargin = 0.1f;

		entity.AddComponent<LuaScriptComponent>().ScriptHandle = 424242;

		auto& animation = entity.AddComponent<AnimationComponent>();
		AnimationClip idle("Idle", 0.2f, /*loop=*/false);
		idle.FrameDefinitions.push_back({ 0, { 0.0f, 0.0f }, { 0.5f, 0.5f } });
		AnimationClip run("Run", 0.05f, /*loop=*/true);
		run.FrameDefinitions.push_back({ 777, { 0.0f, 0.5f }, { 0.25f, 1.0f } });
		run.FrameDefinitions.push_back({ 777, { 0.25f, 0.5f }, { 0.5f, 1.0f } });
		animation.AddClip(idle);
		animation.AddClip(run);
		animation.DefaultClipName = "Run";

		auto& rigidbody = entity.AddComponent<Rigidbody2DComponent>();
		rigidbody.Type = Rigidbody2DComponent::BodyType::Dynamic;
		rigidbody.FixedRotation = true;

		auto& box = entity.AddComponent<BoxCollider2DComponent>();
		box.Offset = { 0.25f, -0.5f };
		box.Size = { 1.0f, 2.0f };
		box.Density = 2.0f;
		box.Friction = 0.1f;
		box.Restitution = 0.8f;
		box.RestitutionThreshold = 1.5f;
		box.CategoryBits = 0x0004;
		box.MaskBits = 0x00F0;
		box.IsTrigger = true;

		auto& circle = entity.AddComponent<CircleCollider2DComponent>();
		circle.Offset = { -1.0f, 1.0f };
		circle.Radius = 0.75f;
		circle.Density = 3.0f;
		circle.Friction = 0.9f;
		circle.Restitution = 0.3f;
		circle.RestitutionThreshold = 2.0f;
		circle.CategoryBits = 0x0008;
		circle.MaskBits = 0x0F00;
		circle.IsTrigger = true;
	}

}

TEST_SUITE("Scene")
{
	TEST_CASE_FIXTURE(Test::SceneFixture, "SceneSerializer round-trip preserves every serialized field")
	{
		m_Scene->SetName("RoundTripScene");
		Entity source = m_Scene->CreateEntity("Player");
		const UUID uuid = source.GetUUID();
		AddAllSerializableComponents(source);

		Ref<Scene> loaded = SerializeRoundTrip(m_Scene);
		CHECK(loaded->GetName() == "RoundTripScene");

		Entity entity = loaded->FindEntityByUUID(uuid);
		REQUIRE(entity);

		SUBCASE("TagComponent")
		{
			CHECK(entity.GetComponent<TagComponent>().Tag == "Player");
		}

		SUBCASE("TransformComponent")
		{
			const auto& transform = entity.GetComponent<TransformComponent>();
			CHECK(transform.Translation == ApproxVec(glm::vec3{ 1.5f, -2.25f, 3.0f }));
			CHECK(transform.Rotation == ApproxVec(glm::vec3{ 0.1f, 0.2f, 0.3f }));
			CHECK(transform.Scale == ApproxVec(glm::vec3{ 2.0f, 3.0f, 4.0f }));
		}

		SUBCASE("SpriteRendererComponent")
		{
			REQUIRE(entity.HasComponent<SpriteRendererComponent>());
			const auto& sprite = entity.GetComponent<SpriteRendererComponent>();
			CHECK(sprite.Color == ApproxVec(glm::vec4{ 0.1f, 0.2f, 0.3f, 0.4f }));
			CHECK((uint64_t)sprite.TextureHandle == 12345);
			CHECK(sprite.TilingFactor == doctest::Approx(2.5f));
			CHECK(sprite.FlipX);
			CHECK(sprite.FlipY);
			CHECK(sprite.SortingLayer == "Foreground");
			CHECK(sprite.SortingOrder == 7);
		}

		SUBCASE("CameraComponent")
		{
			REQUIRE(entity.HasComponent<CameraComponent>());
			const auto& camera = entity.GetComponent<CameraComponent>();
			CHECK(camera.Camera.GetOrthographicSize() == doctest::Approx(15.0f));
			CHECK(camera.Camera.GetOrthographicNearClip() == doctest::Approx(-5.0f));
			CHECK(camera.Camera.GetOrthographicFarClip() == doctest::Approx(5.0f));
			CHECK_FALSE(camera.Primary);
			CHECK(camera.FixedAspectRatio);
			CHECK(camera.DesignWidth == 1280);
			CHECK(camera.DesignHeight == 720);
			CHECK(camera.ShowSafeArea);
			CHECK(camera.SafeAreaMargin == doctest::Approx(0.1f));
			CHECK(camera.Camera.GetAspectRatio() == doctest::Approx(1280.0f / 720.0f));
		}

		SUBCASE("LuaScriptComponent")
		{
			REQUIRE(entity.HasComponent<LuaScriptComponent>());
			CHECK((uint64_t)entity.GetComponent<LuaScriptComponent>().ScriptHandle == 424242);
		}

		SUBCASE("AnimationComponent")
		{
			REQUIRE(entity.HasComponent<AnimationComponent>());
			const auto& animation = entity.GetComponent<AnimationComponent>();
			CHECK(animation.DefaultClipName == "Run");
			CHECK(animation.State.CurrentClipName == "Run");
			REQUIRE(animation.Clips.size() == 2);

			const AnimationClip& idle = animation.Clips.at("Idle");
			CHECK(idle.FrameDuration == doctest::Approx(0.2f));
			CHECK_FALSE(idle.Loop);
			REQUIRE(idle.FrameDefinitions.size() == 1);
			CHECK(idle.FrameDefinitions[0].UVMax == ApproxVec(glm::vec2{ 0.5f, 0.5f }));

			const AnimationClip& run = animation.Clips.at("Run");
			CHECK(run.FrameDuration == doctest::Approx(0.05f));
			CHECK(run.Loop);
			REQUIRE(run.FrameDefinitions.size() == 2);
			CHECK((uint64_t)run.FrameDefinitions[1].TextureHandle == 777);
			CHECK(run.FrameDefinitions[1].UVMin == ApproxVec(glm::vec2{ 0.25f, 0.5f }));
			CHECK(run.FrameDefinitions[1].UVMax == ApproxVec(glm::vec2{ 0.5f, 1.0f }));

			CHECK(run.Frames.empty());
		}

		SUBCASE("Rigidbody2DComponent")
		{
			REQUIRE(entity.HasComponent<Rigidbody2DComponent>());
			const auto& rigidbody = entity.GetComponent<Rigidbody2DComponent>();
			CHECK(rigidbody.Type == Rigidbody2DComponent::BodyType::Dynamic);
			CHECK(rigidbody.FixedRotation);
			CHECK(rigidbody.RuntimeBody == nullptr);
		}

		SUBCASE("BoxCollider2DComponent")
		{
			REQUIRE(entity.HasComponent<BoxCollider2DComponent>());
			const auto& box = entity.GetComponent<BoxCollider2DComponent>();
			CHECK(box.Offset == ApproxVec(glm::vec2{ 0.25f, -0.5f }));
			CHECK(box.Size == ApproxVec(glm::vec2{ 1.0f, 2.0f }));
			CHECK(box.Density == doctest::Approx(2.0f));
			CHECK(box.Friction == doctest::Approx(0.1f));
			CHECK(box.Restitution == doctest::Approx(0.8f));
			CHECK(box.RestitutionThreshold == doctest::Approx(1.5f));
			CHECK(box.CategoryBits == 0x0004);
			CHECK(box.MaskBits == 0x00F0);
			CHECK(box.IsTrigger);
			CHECK(box.RuntimeFixture == nullptr);
		}

		SUBCASE("CircleCollider2DComponent")
		{
			REQUIRE(entity.HasComponent<CircleCollider2DComponent>());
			const auto& circle = entity.GetComponent<CircleCollider2DComponent>();
			CHECK(circle.Offset == ApproxVec(glm::vec2{ -1.0f, 1.0f }));
			CHECK(circle.Radius == doctest::Approx(0.75f));
			CHECK(circle.Density == doctest::Approx(3.0f));
			CHECK(circle.Friction == doctest::Approx(0.9f));
			CHECK(circle.Restitution == doctest::Approx(0.3f));
			CHECK(circle.RestitutionThreshold == doctest::Approx(2.0f));
			CHECK(circle.CategoryBits == 0x0008);
			CHECK(circle.MaskBits == 0x0F00);
			CHECK(circle.IsTrigger);
			CHECK(circle.RuntimeFixture == nullptr);
		}
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "SceneSerializer round-trip does not load registered animation textures")
	{
		const std::filesystem::path assetDirectory = m_TempDirectory.GetPath() / "Assets";
		const std::filesystem::path registryPath = assetDirectory / "AssetRegistry.yregistry";
		m_TempDirectory.WriteFile("Assets/Textures/Hero.png", "not a real image");
		const AssetHandle textureHandle;
		Test::WriteAssetRegistry(registryPath, { Test::MakeAssetMetadata(textureHandle, AssetType::Texture, "Textures/Hero.png") });
		// 必须用内置 Loader：空注册表下纹理本来就加载不了，IsAssetLoaded 的检查会失去意义
		const Ref<RuntimeAssetManager> assetManager = CreateRef<RuntimeAssetManager>(RuntimeAssetManagerSpecification{
			assetDirectory, registryPath, CreateBuiltinAssetLoaders() });
		REQUIRE(assetManager->GetAssetType(textureHandle) == AssetType::Texture);

		SceneContext context;
		context.AssetManager = assetManager;
		m_Scene->SetContext(context);

		Entity source = m_Scene->CreateEntity("Hero");
		AnimationClip walk("Walk");
		walk.FrameDefinitions.push_back({ textureHandle, { 0.0f, 0.0f }, { 0.5f, 1.0f } });
		source.AddComponent<AnimationComponent>().AddClip(walk);

		Ref<Scene> loaded = SerializeRoundTrip(m_Scene);
		Entity entity = loaded->FindEntityByUUID(source.GetUUID());
		REQUIRE(entity);
		const auto& clips = entity.GetComponent<AnimationComponent>().Clips;
		REQUIRE(clips.count("Walk") == 1);

		const AnimationClip& loadedWalk = clips.at("Walk");
		REQUIRE(loadedWalk.FrameDefinitions.size() == 1);
		CHECK((uint64_t)loadedWalk.FrameDefinitions[0].TextureHandle == (uint64_t)textureHandle);
		CHECK(loadedWalk.FrameDefinitions[0].UVMax == ApproxVec(glm::vec2{ 0.5f, 1.0f }));
		CHECK(loadedWalk.Frames.empty());
		CHECK_FALSE(assetManager->IsAssetLoaded(textureHandle));
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "SceneSerializer round-trip preserves hierarchy and child order")
	{
		Entity root = m_Scene->CreateEntity("Root");
		Entity first = m_Scene->CreateChildEntity(root, "First");
		Entity second = m_Scene->CreateChildEntity(root, "Second");
		Entity grandChild = m_Scene->CreateChildEntity(first, "GrandChild");

		Ref<Scene> loaded = SerializeRoundTrip(m_Scene);
		CHECK(loaded->GetAllEntitiesWith<IDComponent>().size() == 4);

		Entity loadedRoot = loaded->FindEntityByUUID(root.GetUUID());
		Entity loadedFirst = loaded->FindEntityByUUID(first.GetUUID());
		Entity loadedGrandChild = loaded->FindEntityByUUID(grandChild.GetUUID());
		REQUIRE(loadedRoot);
		REQUIRE(loadedFirst);
		REQUIRE(loadedGrandChild);

		CHECK((uint64_t)loadedRoot.GetParentUUID() == 0);
		REQUIRE(loadedRoot.Children().size() == 2);
		CHECK((uint64_t)loadedRoot.Children()[0] == (uint64_t)first.GetUUID());
		CHECK((uint64_t)loadedRoot.Children()[1] == (uint64_t)second.GetUUID());
		CHECK((uint64_t)loadedFirst.GetParentUUID() == (uint64_t)root.GetUUID());
		CHECK((uint64_t)loadedGrandChild.GetParentUUID() == (uint64_t)first.GetUUID());
		CHECK(loadedGrandChild.IsDescendantOf(loadedRoot));
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "SceneSerializer rejects missing or invalid scene files")
	{
		Ref<Scene> scene = CreateRef<Scene>();

		SUBCASE("missing file")
		{
			CHECK_FALSE(SceneSerializer(scene).Deserialize(m_TempDirectory.GetPath() / "DoesNotExist.yui"));
		}

		SUBCASE("file without Scene root node")
		{
			const auto path = m_TempDirectory.WriteFile("NotAScene.yui", "Project:\n  Name: Foo\n");
			CHECK_FALSE(SceneSerializer(scene).Deserialize(path));
		}

		CHECK(scene->GetAllEntitiesWith<IDComponent>().size() == 0);
	}

	// round-trip 发现不了“读写两侧同时改了键名”导致的旧场景无法打开，因此另用一份提交在仓库里的参考文件兜底。
	// 有意修改场景格式时，同步更新 data/Scenes/Reference.yui 与本用例。
	TEST_CASE_FIXTURE(Test::SceneFixture, "SceneSerializer loads the checked-in reference scene")
	{
		const std::filesystem::path referencePath = std::filesystem::path(YUICY_TEST_DATA_DIR) / "Scenes" / "Reference.yui";
		Ref<Scene> scene = CreateRef<Scene>();
		REQUIRE(SceneSerializer(scene).Deserialize(referencePath));

		CHECK(scene->GetName() == "ReferenceScene");
		CHECK(scene->GetAllEntitiesWith<IDComponent>().size() == 2);

		Entity root = scene->FindEntityByUUID(1001);
		Entity child = scene->FindEntityByUUID(1002);
		REQUIRE(root);
		REQUIRE(child);

		CHECK(root.GetComponent<TagComponent>().Tag == "Root");
		REQUIRE(root.Children().size() == 1);
		CHECK((uint64_t)root.Children()[0] == 1002);
		CHECK((uint64_t)child.GetParentUUID() == 1001);

		const auto& transform = root.GetComponent<TransformComponent>();
		CHECK(transform.Translation == ApproxVec(glm::vec3{ 1.0f, 2.0f, 3.0f }));
		CHECK(transform.Rotation == ApproxVec(glm::vec3{ 0.0f, 0.0f, 0.5f }));
		CHECK(transform.Scale == ApproxVec(glm::vec3{ 2.0f, 2.0f, 1.0f }));

		REQUIRE(root.HasComponent<SpriteRendererComponent>());
		const auto& sprite = root.GetComponent<SpriteRendererComponent>();
		CHECK(sprite.Color == ApproxVec(glm::vec4{ 1.0f, 0.5f, 0.25f, 1.0f }));
		CHECK(sprite.TilingFactor == doctest::Approx(2.0f));
		CHECK(sprite.FlipX);
		CHECK(sprite.SortingLayer == "Foreground");
		CHECK(sprite.SortingOrder == 3);

		REQUIRE(root.HasComponent<CameraComponent>());
		const auto& camera = root.GetComponent<CameraComponent>();
		CHECK(camera.Camera.GetOrthographicSize() == doctest::Approx(8.0f));
		CHECK(camera.Primary);
		CHECK(camera.DesignWidth == 640);
		CHECK(camera.DesignHeight == 480);

		REQUIRE(child.HasComponent<Rigidbody2DComponent>());
		CHECK(child.GetComponent<Rigidbody2DComponent>().Type == Rigidbody2DComponent::BodyType::Dynamic);
		CHECK(child.GetComponent<Rigidbody2DComponent>().FixedRotation);

		REQUIRE(child.HasComponent<BoxCollider2DComponent>());
		CHECK(child.GetComponent<BoxCollider2DComponent>().Size == ApproxVec(glm::vec2{ 0.5f, 1.0f }));
		CHECK(child.GetComponent<BoxCollider2DComponent>().MaskBits == 0x0003);

		REQUIRE(child.HasComponent<CircleCollider2DComponent>());
		CHECK(child.GetComponent<CircleCollider2DComponent>().Radius == doctest::Approx(0.25f));

		REQUIRE(child.HasComponent<LuaScriptComponent>());
		CHECK((uint64_t)child.GetComponent<LuaScriptComponent>().ScriptHandle == 5005);

		REQUIRE(child.HasComponent<AnimationComponent>());
		const auto& animation = child.GetComponent<AnimationComponent>();
		CHECK(animation.DefaultClipName == "Walk");
		REQUIRE(animation.Clips.count("Walk") == 1);
		CHECK(animation.Clips.at("Walk").FrameDuration == doctest::Approx(0.125f));
		CHECK_FALSE(animation.Clips.at("Walk").Loop);
		CHECK(animation.Clips.at("Walk").FrameDefinitions.size() == 1);
	}
}
