#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Window.h"
#include "Yuicy/Core/LayerStack.h"
#include "Yuicy/ImGui/ImGuiLayer.h"

namespace Yuicy {
	class YUICY_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);
		bool OnWindowClose(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *_window; }

		static Application& Get() { return *_instance; }

	private:
		std::unique_ptr<Window>		_window;
		ImGuiLayer*					_imGuiLayer;
		bool						_running = true;
		LayerStack					_layerStack;

	private:
		static Application* _instance;
	};

	Application* CreateApplication();
}
