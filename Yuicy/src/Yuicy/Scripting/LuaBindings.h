#pragma once

#include <sol/sol.hpp>

namespace Yuicy {
	namespace LuaBindings
	{
		// Register all bindings
		void RegisterAll(sol::state& lua);
	}

}
