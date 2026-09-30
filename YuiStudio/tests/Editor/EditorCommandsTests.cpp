#include "pch.h"

#include "Editor/Commands/AddComponentCommand.h"
#include "Editor/Commands/CreateEntityCommand.h"
#include "Editor/Commands/RemoveComponentCommand.h"
#include "Editor/Commands/ReparentEntityCommand.h"
#include "Editor/Commands/SetTransformCommand.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

namespace {

	size_t EntityCount(Scene& scene)
	{
		return scene.GetAllEntitiesWith<IDComponent>().size();
	}

}

TEST_SUITE("Editor")
{
	TEST_CASE_FIXTURE(Test::SceneFixture, "CreateEntityCommand Execute Undo Redo")
	{
		EditorCommandHistory history;
		history.ExecuteCommandT<CreateEntityCommand>(m_Scene.get(), "Created");
		CHECK(EntityCount(*m_Scene) == 1);
		CHECK(m_Scene->FindEntityByName("Created"));

		history.Undo();
		CHECK(EntityCount(*m_Scene) == 0);

		history.Redo();
		CHECK(EntityCount(*m_Scene) == 1);
		CHECK(m_Scene->FindEntityByName("Created"));
	}

	// CreateEntityCommand 每次 Execute 都生成新 UUID，重做后栈中后续命令按旧 UUID 找不到实体
	// TODO: 删除 should_fail（CreateEntityCommand 重做时复用首次生成的 UUID 之后）
	TEST_CASE_FIXTURE(Test::SceneFixture, "Redo of CreateEntityCommand keeps later commands valid" * doctest::should_fail())
	{
		EditorCommandHistory history;
		auto create = CreateScope<CreateEntityCommand>(m_Scene.get(), "Created");
		CreateEntityCommand* createCommand = create.get();
		history.ExecuteCommand(std::move(create));
		const UUID createdUUID = createCommand->GetCreatedUUID();

		history.ExecuteCommandT<AddComponentCommand<SpriteRendererComponent>>(m_Scene.get(), createdUUID);
		history.Undo();
		history.Undo();
		history.Redo();
		history.Redo();

		Entity entity = m_Scene->FindEntityByName("Created");
		REQUIRE(entity);
		CHECK((uint64_t)entity.GetUUID() == (uint64_t)createdUUID);
		CHECK(entity.HasComponent<SpriteRendererComponent>());
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "CreateChildEntityCommand attaches to the parent")
	{
		Entity parent = m_Scene->CreateEntity("Parent");
		EditorCommandHistory history;

		auto create = CreateScope<CreateChildEntityCommand>(m_Scene.get(), parent.GetUUID(), "Child");
		CreateChildEntityCommand* createCommand = create.get();
		history.ExecuteCommand(std::move(create));

		Entity child = m_Scene->FindEntityByUUID(createCommand->GetCreatedUUID());
		REQUIRE(child);
		CHECK((uint64_t)child.GetParentUUID() == (uint64_t)parent.GetUUID());
		CHECK(parent.Children().size() == 1);

		history.Undo();
		CHECK(parent.Children().empty());
		CHECK(EntityCount(*m_Scene) == 1);
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "AddComponentCommand and RemoveComponentCommand are symmetric")
	{
		Entity entity = m_Scene->CreateEntity("Entity");
		const UUID uuid = entity.GetUUID();
		EditorCommandHistory history;

		history.ExecuteCommandT<AddComponentCommand<BoxCollider2DComponent>>(m_Scene.get(), uuid);
		REQUIRE(entity.HasComponent<BoxCollider2DComponent>());
		entity.GetComponent<BoxCollider2DComponent>().Size = { 3.0f, 4.0f };

		history.ExecuteCommandT<RemoveComponentCommand<BoxCollider2DComponent>>(m_Scene.get(), uuid);
		CHECK_FALSE(entity.HasComponent<BoxCollider2DComponent>());

		// Remove 的 Undo 必须恢复删除前的字段值，而不是一个默认构造的组件
		history.Undo();
		REQUIRE(entity.HasComponent<BoxCollider2DComponent>());
		CHECK(entity.GetComponent<BoxCollider2DComponent>().Size == ApproxVec(glm::vec2{ 3.0f, 4.0f }));

		history.Undo();
		CHECK_FALSE(entity.HasComponent<BoxCollider2DComponent>());

		history.Redo();
		CHECK(entity.HasComponent<BoxCollider2DComponent>());
		history.Redo();
		CHECK_FALSE(entity.HasComponent<BoxCollider2DComponent>());
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "ReparentEntityCommand Execute Undo Redo keep world position")
	{
		Entity parent = m_Scene->CreateEntity("Parent");
		parent.GetComponent<TransformComponent>().Translation = { 10.0f, 0.0f, 0.0f };
		Entity entity = m_Scene->CreateEntity("Entity");
		entity.GetComponent<TransformComponent>().Translation = { 1.0f, 1.0f, 0.0f };
		EditorCommandHistory history;

		history.ExecuteCommandT<ReparentEntityCommand>(m_Scene.get(), entity.GetUUID(), parent.GetUUID());
		CHECK((uint64_t)entity.GetParentUUID() == (uint64_t)parent.GetUUID());
		CHECK(m_Scene->GetWorldSpaceTransform(entity).Translation == ApproxVec(glm::vec3{ 1.0f, 1.0f, 0.0f }));

		history.Undo();
		CHECK((uint64_t)entity.GetParentUUID() == 0);
		CHECK(parent.Children().empty());
		CHECK(entity.GetComponent<TransformComponent>().Translation == ApproxVec(glm::vec3{ 1.0f, 1.0f, 0.0f }));

		history.Redo();
		CHECK((uint64_t)entity.GetParentUUID() == (uint64_t)parent.GetUUID());
		CHECK(parent.Children().size() == 1);
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "SetTransformCommand Execute Undo Redo")
	{
		Entity entity = m_Scene->CreateEntity("Entity");
		const glm::vec3 oldTranslation{ 0.0f }, oldRotation{ 0.0f }, oldScale{ 1.0f };
		const glm::vec3 newTranslation{ 1.0f, 2.0f, 3.0f }, newRotation{ 0.0f, 0.0f, 1.0f }, newScale{ 2.0f, 2.0f, 2.0f };
		EditorCommandHistory history;

		history.ExecuteCommandT<SetTransformCommand>(m_Scene.get(), entity.GetUUID(),
			oldTranslation, oldRotation, oldScale, newTranslation, newRotation, newScale);
		const auto& transform = entity.GetComponent<TransformComponent>();
		CHECK(transform.Translation == ApproxVec(newTranslation));
		CHECK(transform.Rotation == ApproxVec(newRotation));
		CHECK(transform.Scale == ApproxVec(newScale));

		history.Undo();
		CHECK(transform.Translation == ApproxVec(oldTranslation));
		CHECK(transform.Rotation == ApproxVec(oldRotation));
		CHECK(transform.Scale == ApproxVec(oldScale));

		history.Redo();
		CHECK(transform.Translation == ApproxVec(newTranslation));
	}
}
