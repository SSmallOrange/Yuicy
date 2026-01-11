#pragma once

#include <sol/sol.hpp>

namespace Yuicy {

	class Scene;

	namespace LuaBindings
	{
		// Register all bindings
		void RegisterAll(sol::state& lua);

		// Individual binding functions
		void RegisterMath(sol::state& lua);        // glm::vec2, vec3, vec4
		void RegisterInput(sol::state& lua);       // Input::IsKeyPressed, Key codes
		void RegisterEntity(sol::state& lua);      // Entity class
		void RegisterComponents(sol::state& lua);  // TransformComponent, SpriteRendererComponent, etc.
		void RegisterScene(sol::state& lua);       // Scene access
		void RegisterLog(sol::state& lua);         // Logging functions
	}

}
