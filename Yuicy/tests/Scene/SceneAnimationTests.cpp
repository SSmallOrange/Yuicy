#include "pch.h"

#include "Yuicy/Asset/AssetLoader.h"
#include "Yuicy/Asset/EditorAssetManager.h"
#include "Yuicy/Renderer/Texture.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

namespace {

	// 不创建 GPU 资源
	class FakeTexture2D : public Texture2D
	{
	public:
		uint32_t GetWidth() const override { return 64; }
		uint32_t GetHeight() const override { return 64; }
		void SetData(void* data, uint32_t size) override {}
		void Bind(uint32_t slot = 0) const override {}
		uint32_t GetRendererID() override { return 0; }
		bool operator==(const Texture& other) const override { return this == &other; }
	};

	// SceneContext 只持有资源管理器的弱引用，由夹具负责持有
	struct AnimatedSceneFixture
	{
		AnimatedSceneFixture()
		{
			const std::filesystem::path assetDirectory = TempDirectory.GetPath() / "Assets";
			std::filesystem::create_directories(assetDirectory);
			AssetManager = CreateRef<EditorAssetManager>(EditorAssetManagerSpecification{
				assetDirectory, assetDirectory / "AssetRegistry.yregistry", CreateRef<AssetLoaderRegistry>() });

			Texture = CreateRef<FakeTexture2D>();
			TextureHandle = AssetManager->AddMemoryOnlyAsset(Texture);

			SceneContext context;
			context.AssetManager = AssetManager;
			EditorScene->SetContext(context);

			Hero = EditorScene->CreateEntity("Hero");
			Hero.AddComponent<SpriteRendererComponent>();

			AnimationClip walk("Walk");
			walk.FrameDefinitions.push_back({ TextureHandle, { 0.0f, 0.0f }, { 0.5f, 1.0f } });
			// 未登记的 Handle
			walk.FrameDefinitions.push_back({ AssetHandle(), { 0.5f, 0.0f }, { 1.0f, 1.0f } });
			Hero.AddComponent<AnimationComponent>().AddClip(walk);
		}

		static AnimationClip& GetWalkClip(Entity entity) { return entity.GetComponent<AnimationComponent>().Clips.at("Walk"); }

		Test::ScopedTempDirectory TempDirectory;
		Ref<EditorAssetManager> AssetManager;
		Ref<FakeTexture2D> Texture;
		AssetHandle TextureHandle = 0;
		Ref<Scene> EditorScene = CreateRef<Scene>();
		Entity Hero;
	};

}

TEST_SUITE("Scene")
{
	TEST_CASE_FIXTURE(AnimatedSceneFixture, "Scene resolves animation frames from its context when the runtime starts")
	{
		Ref<Scene> runtimeScene = Scene::Copy(EditorScene);
		Entity runtimeHero = runtimeScene->FindEntityByUUID(Hero.GetUUID());
		REQUIRE(runtimeHero);

		runtimeScene->OnRuntimeStart();

		const AnimationClip& clip = GetWalkClip(runtimeHero);
		REQUIRE(clip.Frames.size() == 2);
		REQUIRE(clip.Frames[0]);
		CHECK(clip.Frames[0]->GetTexture() == Texture);
		CHECK(clip.Frames[0]->GetTexCoords()[2] == ApproxVec(glm::vec2{ 0.5f, 1.0f }));
		CHECK(clip.Frames[1] == nullptr);
		CHECK(GetWalkClip(Hero).Frames.empty());

		runtimeScene->OnRuntimeStop();
	}

	TEST_CASE_FIXTURE(AnimatedSceneFixture, "Scene re-resolves animation frames whose definitions changed while running")
	{
		EditorScene->OnRuntimeStart();

		AnimationClip& clip = GetWalkClip(Hero);
		const AnimationFrameDefinition insertedFrame{ TextureHandle, { 0.25f, 0.0f }, { 0.75f, 1.0f } };
		clip.FrameDefinitions.insert(clip.FrameDefinitions.begin(), insertedFrame);
		clip.Frames.clear();

		// 场景中没有主相机，OnUpdateRuntime 不会调用 Renderer2D
		EditorScene->OnUpdateRuntime(Timestep(0.0f));

		REQUIRE(clip.Frames.size() == 3);
		REQUIRE(clip.Frames[0]);
		CHECK(clip.Frames[0]->GetTexCoords()[0] == ApproxVec(glm::vec2{ 0.25f, 0.0f }));
		CHECK(Hero.GetComponent<SpriteRendererComponent>().SubTexture == clip.Frames[0]);

		EditorScene->OnRuntimeStop();
	}

	TEST_CASE_FIXTURE(AnimatedSceneFixture, "Scene without an asset manager resolves every animation frame to null")
	{
		EditorScene->SetContext({});
		EditorScene->OnSimulationStart();

		const AnimationClip& clip = GetWalkClip(Hero);
		REQUIRE(clip.Frames.size() == clip.FrameDefinitions.size());
		CHECK(clip.Frames[0] == nullptr);
		CHECK(clip.Frames[1] == nullptr);

		EditorScene->OnSimulationStop();
	}
}
