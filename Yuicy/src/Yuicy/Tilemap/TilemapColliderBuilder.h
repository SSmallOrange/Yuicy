#pragma once

#include "Yuicy/Scene/Components.h"

class b2World;

namespace Yuicy {

	class Entity;
	class Scene;

	class TilemapColliderBuilder
	{
	public:
		static void Build(Scene* scene, b2World* world, Entity tilemapEntity);
		static void ClearRuntimeData(TilemapCollider2DComponent& collider);
	};

}
