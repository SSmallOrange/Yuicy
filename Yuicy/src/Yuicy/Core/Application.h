#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Window.h"

namespace Yuicy {
	class YUICY_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		std::unique_ptr<Window> _window;
		bool					_running = true;
	};

	Application* CreateApplication();
}
