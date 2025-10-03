#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Window.h"
#include "Yuicy/Core/LayerStack.h"

namespace Yuicy {
	class YUICY_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		bool OnWindowClose(Event& e);

	private:
		std::unique_ptr<Window> _window;
		bool					_running = true;
		LayerStack				_layerStack;
	};

	Application* CreateApplication();
}
