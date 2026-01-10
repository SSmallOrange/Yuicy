#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Window.h"
#include "Yuicy/Core/LayerStack.h"
#include "Yuicy/ImGui/ImGuiLayer.h"

namespace Yuicy {
	class Application
	{
	public:
		Application(const WindowProps& props = WindowProps());
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* layer);

		Window& GetWindow() { return *_window; }

		static Application& Get() { return *_instance; }

	private:
		bool OnWindowClose(Event& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		std::unique_ptr<Window>		_window;
		ImGuiLayer*					_imGuiLayer;
		bool						_minimized = false;
		bool						_running = true;
		LayerStack					_layerStack;
		float						_lastFrameTime = 0.0f;

	private:
		static Application* _instance;
	};

	Application* CreateApplication();
}
