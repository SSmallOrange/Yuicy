#pragma once

#include "Core.h"

namespace Yuicy {
	class YUICY_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
	};

	Application* CreateApplication();
}
