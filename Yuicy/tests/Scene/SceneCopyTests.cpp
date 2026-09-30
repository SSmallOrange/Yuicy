#include "pch.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

TEST_SUITE("Scene")
{
	TEST_CASE_FIXTURE(Test::SceneFixture, "Scene::Copy keeps UUIDs components and hierarchy")
	{
		m_Scene->SetName("Original");
		Entity parent = m_Scene->CreateEntity("Parent");
		Entity child = m_Scene->CreateChildEntity(parent, "Child");

		parent.GetComponent<TransformComponent>().Translation = { 1.0f, 2.0f, 3.0f };
		parent.AddComponent<SpriteRendererComponent>(glm::vec4{ 0.2f, 0.4f, 0.6f, 1.0f });
		child.AddComponent<BoxCollider2DComponent>().Size = { 3.0f, 4.0f };
		child.AddComponent<Rigidbody2DComponent>().Type = Rigidbody2DComponent::BodyType::Kinematic;

		Ref<Scene> copy = Scene::Copy(m_Scene);
		REQUIRE(copy);
		CHECK(copy->GetName() == "Original");
		CHECK(copy->GetAllEntitiesWith<IDComponent>().size() == 2);

		Entity copiedParent = copy->FindEntityByUUID(parent.GetUUID());
		Entity copiedChild = copy->FindEntityByUUID(child.GetUUID());
		REQUIRE(copiedParent);
		REQUIRE(copiedChild);

		CHECK(copiedParent.GetComponent<TagComponent>().Tag == "Parent");
		CHECK(copiedParent.GetComponent<TransformComponent>().Translation == ApproxVec(glm::vec3{ 1.0f, 2.0f, 3.0f }));
		REQUIRE(copiedParent.HasComponent<SpriteRendererComponent>());
		CHECK(copiedParent.GetComponent<SpriteRendererComponent>().Color == ApproxVec(glm::vec4{ 0.2f, 0.4f, 0.6f, 1.0f }));

		REQUIRE(copiedChild.HasComponent<BoxCollider2DComponent>());
		CHECK(copiedChild.GetComponent<BoxCollider2DComponent>().Size == ApproxVec(glm::vec2{ 3.0f, 4.0f }));
		REQUIRE(copiedChild.HasComponent<Rigidbody2DComponent>());
		CHECK(copiedChild.GetComponent<Rigidbody2DComponent>().Type == Rigidbody2DComponent::BodyType::Kinematic);

		CHECK((uint64_t)copiedChild.GetParentUUID() == (uint64_t)parent.GetUUID());
		REQUIRE(copiedParent.Children().size() == 1);
		CHECK((uint64_t)copiedParent.Children()[0] == (uint64_t)child.GetUUID());
	}

	// Play 模式在副本上运行，副本的改动泄漏回编辑场景会导致 Stop 后无法还原
	TEST_CASE_FIXTURE(Test::SceneFixture, "Scene::Copy produces an independent scene")
	{
		Entity entity = m_Scene->CreateEntity("Entity");
		entity.GetComponent<TransformComponent>().Translation = { 1.0f, 0.0f, 0.0f };

		Ref<Scene> copy = Scene::Copy(m_Scene);
		Entity copied = copy->FindEntityByUUID(entity.GetUUID());
		REQUIRE(copied);

		copied.GetComponent<TransformComponent>().Translation = { 9.0f, 9.0f, 9.0f };
		copy->CreateEntity("OnlyInCopy");
		copy->DestroyEntity(copied);

		CHECK(entity.GetComponent<TransformComponent>().Translation == ApproxVec(glm::vec3{ 1.0f, 0.0f, 0.0f }));
		CHECK(m_Scene->GetAllEntitiesWith<IDComponent>().size() == 1);
		CHECK(m_Scene->FindEntityByUUID(entity.GetUUID()));
	}
}
