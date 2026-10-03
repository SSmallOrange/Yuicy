#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Window.h"
#include "Yuicy/Core/LayerStack.h"
#include "Yuicy/ImGui/ImGuiLayer.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <vector>

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

		void SubmitToMainThread(std::function<void()> task);

		Window& GetWindow() { return *_window; }
		ImGuiLayer* GetImGuiLayer() { return _imGuiLayer; }

		static Application& Get() { return *_instance; }

	private:
		bool OnWindowClose(Event& e);
		bool OnWindowResize(WindowResizeEvent& e);
		bool OnWindowFramebufferResize(WindowFramebufferResizeEvent& e);

		void ExecuteMainThreadQueue();

	private:
		std::unique_ptr<Window>		_window;
		ImGuiLayer*					_imGuiLayer;
		bool						_minimized = false;
		bool						_running = true;
		// Thread
		std::vector<std::function<void()>> _mainThreadQueue;
		std::mutex _mainThreadQueueMutex;
		
		LayerStack					_layerStack;
		std::chrono::steady_clock::time_point _lastFrameTime;

	private:
		static Application* _instance;
	};

	Application* CreateApplication();
}
