#include "pch.h"

using namespace Yuicy;
using Yuicy::Test::ApproxVec;

namespace {

	// 带平移、旋转、非 1 缩放的父实体，能暴露局部 / 世界空间换算中遗漏的任一分量
	Entity CreateTransformedParent(Scene& scene)
	{
		Entity parent = scene.CreateEntity("Parent");
		auto& transform = parent.GetComponent<TransformComponent>();
		transform.Translation = { 5.0f, -1.0f, 0.0f };
		transform.Rotation = { 0.0f, 0.0f, glm::radians(90.0f) };
		transform.Scale = { 2.0f, 2.0f, 1.0f };
		return parent;
	}

	bool ContainsUUID(const std::vector<UUID>& uuids, UUID uuid)
	{
		return std::ranges::find(uuids, uuid) != uuids.end();
	}

}

TEST_SUITE("Scene")
{
	TEST_CASE_FIXTURE(Test::SceneFixture, "ParentEntity and UnparentEntity keep the world transform")
	{
		Entity parent = CreateTransformedParent(*m_Scene);
		Entity child = m_Scene->CreateEntity("Child");
		child.GetComponent<TransformComponent>().Translation = { 1.0f, 2.0f, 0.0f };

		m_Scene->ParentEntity(child, parent);

		CHECK((uint64_t)child.GetParentUUID() == (uint64_t)parent.GetUUID());
		CHECK(ContainsUUID(parent.Children(), child.GetUUID()));
		CHECK(m_Scene->GetWorldSpaceTransform(child).Translation == ApproxVec(glm::vec3{ 1.0f, 2.0f, 0.0f }));
		CHECK(m_Scene->GetWorldSpaceTransform(child).Scale == ApproxVec(glm::vec3{ 1.0f, 1.0f, 1.0f }));

		m_Scene->UnparentEntity(child);

		CHECK((uint64_t)child.GetParentUUID() == 0);
		CHECK_FALSE(ContainsUUID(parent.Children(), child.GetUUID()));
		const auto& transform = child.GetComponent<TransformComponent>();
		CHECK(transform.Translation == ApproxVec(glm::vec3{ 1.0f, 2.0f, 0.0f }));
		CHECK(transform.Rotation == ApproxVec(glm::vec3{ 0.0f, 0.0f, 0.0f }));
		CHECK(transform.Scale == ApproxVec(glm::vec3{ 1.0f, 1.0f, 1.0f }));
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "ParentEntity onto a descendant does not create a cycle")
	{
		Entity a = m_Scene->CreateEntity("A");
		Entity b = m_Scene->CreateChildEntity(a, "B");

		m_Scene->ParentEntity(a, b);

		CHECK((uint64_t)b.GetParentUUID() == 0);
		CHECK((uint64_t)a.GetParentUUID() == (uint64_t)b.GetUUID());
		CHECK(ContainsUUID(b.Children(), a.GetUUID()));
		CHECK_FALSE(ContainsUUID(a.Children(), b.GetUUID()));
		CHECK_FALSE(b.IsDescendantOf(a));
	}

	TEST_CASE_FIXTURE(Test::SceneFixture, "DestroyEntity removes descendants and detaches from parent")
	{
		Entity root = m_Scene->CreateEntity("Root");
		Entity middle = m_Scene->CreateChildEntity(root, "Middle");
		Entity leaf = m_Scene->CreateChildEntity(middle, "Leaf");
		Entity sibling = m_Scene->CreateChildEntity(root, "Sibling");

		const UUID middleUUID = middle.GetUUID();
		const UUID leafUUID = leaf.GetUUID();

		m_Scene->DestroyEntity(middle);

		CHECK_FALSE(m_Scene->FindEntityByUUID(middleUUID));
		CHECK_FALSE(m_Scene->FindEntityByUUID(leafUUID));
		CHECK(m_Scene->FindEntityByUUID(sibling.GetUUID()));
		CHECK_FALSE(ContainsUUID(root.Children(), middleUUID));
		CHECK(root.Children().size() == 1);
		CHECK(m_Scene->GetAllEntitiesWith<IDComponent>().size() == 2);
	}
}
