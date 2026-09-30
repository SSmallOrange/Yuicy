#include "pch.h"

#include "Editor/Commands/DeleteEntityCommand.h"
#include "Editor/Commands/EntitySnapshot.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

namespace {

	// 仅用于验证快照会清空运行时指针，从不解引用
	void* const kFakeRuntimeHandle = reinterpret_cast<void*>(0x1);

	bool ContainsUUID(const std::vector<UUID>& uuids, UUID uuid)
	{
		return std::ranges::find(uuids, uuid) != uuids.end();
	}

}

TEST_SUITE("Editor")
{
	// TODO: 改为遍历全部组件类型，新增组件未加入快照时让本用例失败（组件注册表落地后，见 AI_INFRA_ROADMAP.md 3.2）
	TEST_CASE_FIXTURE(Test::SceneFixture, "EntitySnapshot Capture and Restore keep every component")
	{
		Entity entity = m_Scene->CreateEntity("Snapshot");
		const UUID uuid = entity.GetUUID();

		entity.GetComponent<TransformComponent>().Translation = { 1.0f, 2.0f, 3.0f };
		entity.AddComponent<SpriteRendererComponent>().SortingOrder = 5;
		entity.AddComponent<CameraComponent>().DesignWidth = 800;
		entity.AddComponent<LuaScriptComponent>().ScriptHandle = 99;

		auto& animation = entity.AddComponent<AnimationComponent>();
		animation.AddClip(AnimationClip("Jump", 0.3f, /*loop=*/false));

		auto& rigidbody = entity.AddComponent<Rigidbody2DComponent>();
		rigidbody.Type = Rigidbody2DComponent::BodyType::Dynamic;
		rigidbody.RuntimeBody = kFakeRuntimeHandle;

		auto& box = entity.AddComponent<BoxCollider2DComponent>();
		box.Size = { 2.0f, 3.0f };
		box.RuntimeFixture = kFakeRuntimeHandle;

		auto& circle = entity.AddComponent<CircleCollider2DComponent>();
		circle.Radius = 4.0f;
		circle.RuntimeFixture = kFakeRuntimeHandle;

		const EntitySnapshot snapshot = EntitySnapshot::Capture(entity);
		m_Scene->DestroyEntity(entity);
		REQUIRE_FALSE(m_Scene->FindEntityByUUID(uuid));

		Entity restored = snapshot.Restore(m_Scene.get());
		REQUIRE(restored);
		CHECK((uint64_t)restored.GetUUID() == (uint64_t)uuid);
		CHECK(m_Scene->FindEntityByUUID(uuid) == restored);
		CHECK(restored.GetComponent<TagComponent>().Tag == "Snapshot");
		CHECK(restored.GetComponent<TransformComponent>().Translation == ApproxVec(glm::vec3{ 1.0f, 2.0f, 3.0f }));

		REQUIRE(restored.HasComponent<SpriteRendererComponent>());
		CHECK(restored.GetComponent<SpriteRendererComponent>().SortingOrder == 5);
		REQUIRE(restored.HasComponent<CameraComponent>());
		CHECK(restored.GetComponent<CameraComponent>().DesignWidth == 800);
		REQUIRE(restored.HasComponent<LuaScriptComponent>());
		CHECK((uint64_t)restored.GetComponent<LuaScriptComponent>().ScriptHandle == 99);
		REQUIRE(restored.HasComponent<AnimationComponent>());
		CHECK(restored.GetComponent<AnimationComponent>().Clips.count("Jump") == 1);

		// Box2D 对象属于运行时物理世界，恢复到编辑场景后必须由 OnRuntimeStart 重新创建
		REQUIRE(restored.HasComponent<Rigidbody2DComponent>());
		CHECK(restored.GetComponent<Rigidbody2DComponent>().Type == Rigidbody2DComponent::BodyType::Dynamic);
		CHECK(restored.GetComponent<Rigidbody2DComponent>().RuntimeBody == nullptr);
		REQUIRE(restored.HasComponent<BoxCollider2DComponent>());
		CHECK(restored.GetComponent<BoxCollider2DComponent>().Size == ApproxVec(glm::vec2{ 2.0f, 3.0f }));
		CHECK(restored.GetComponent<BoxCollider2DComponent>().RuntimeFixture == nullptr);
		REQUIRE(restored.HasComponent<CircleCollider2DComponent>());
		CHECK(restored.GetComponent<CircleCollider2DComponent>().Radius == doctest::Approx(4.0f));
		CHECK(restored.GetComponent<CircleCollider2DComponent>().RuntimeFixture == nullptr);
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "DeleteEntityCommand Undo restores the whole subtree")
	{
		Entity outer = m_Scene->CreateEntity("Outer");
		Entity root = m_Scene->CreateChildEntity(outer, "Root");
		Entity child = m_Scene->CreateChildEntity(root, "Child");
		Entity grandChild = m_Scene->CreateChildEntity(child, "GrandChild");
		child.AddComponent<SpriteRendererComponent>().SortingLayer = "UI";

		const UUID outerUUID = outer.GetUUID();
		const UUID rootUUID = root.GetUUID();
		const UUID childUUID = child.GetUUID();
		const UUID grandChildUUID = grandChild.GetUUID();

		EditorCommandHistory history;
		history.ExecuteCommandT<DeleteEntityCommand>(m_Scene.get(), root);

		CHECK_FALSE(m_Scene->FindEntityByUUID(rootUUID));
		CHECK_FALSE(m_Scene->FindEntityByUUID(childUUID));
		CHECK_FALSE(m_Scene->FindEntityByUUID(grandChildUUID));
		CHECK_FALSE(ContainsUUID(m_Scene->FindEntityByUUID(outerUUID).Children(), rootUUID));

		history.Undo();

		Entity restoredRoot = m_Scene->FindEntityByUUID(rootUUID);
		Entity restoredChild = m_Scene->FindEntityByUUID(childUUID);
		Entity restoredGrandChild = m_Scene->FindEntityByUUID(grandChildUUID);
		REQUIRE(restoredRoot);
		REQUIRE(restoredChild);
		REQUIRE(restoredGrandChild);

		CHECK((uint64_t)restoredRoot.GetParentUUID() == (uint64_t)outerUUID);
		CHECK(ContainsUUID(m_Scene->FindEntityByUUID(outerUUID).Children(), rootUUID));
		CHECK(ContainsUUID(restoredRoot.Children(), childUUID));
		CHECK(ContainsUUID(restoredChild.Children(), grandChildUUID));
		CHECK((uint64_t)restoredGrandChild.GetParentUUID() == (uint64_t)childUUID);
		REQUIRE(restoredChild.HasComponent<SpriteRendererComponent>());
		CHECK(restoredChild.GetComponent<SpriteRendererComponent>().SortingLayer == "UI");

		history.Redo();

		CHECK_FALSE(m_Scene->FindEntityByUUID(rootUUID));
		CHECK_FALSE(m_Scene->FindEntityByUUID(grandChildUUID));
		CHECK(m_Scene->GetAllEntitiesWith<IDComponent>().size() == 1);
	}
}
