#include "pch.h"

#include "Yuicy/Core/Application.h"
#include "Yuicy/Events/ApplicationEvent.h"

#include "Yuicy/Renderer/Renderer.h"
#include <glfw/glfw3.h>

#include <glad/glad.h>

namespace Yuicy {
	Application* Application::_instance = nullptr;

	Application::Application() { 
		YUICY_PROFILE_FUNCTION();
		YUICY_ASSERT(!_instance, "Application already exists!");
		_instance = this;

		_window = Window::Create(WindowProps());
		_window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

		Renderer::Init();

		_imGuiLayer = new ImGuiLayer();
		PushOverlay(_imGuiLayer);
	}

	Application::~Application() {

	}

	void Application::OnEvent(Event& e) {
		// HZ_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));

		for (auto it = _layerStack.rbegin(); it != _layerStack.rend(); ++it) {  // 事件反向冒泡
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}

		// YUICY_CORE_INFO("EventInfo:{}", e.ToString());
	}

	bool Application::OnWindowClose(Event& e) {
		_running = false;
		return true;
	}

	void Application::Run() {

		WindowResizeEvent e(1280, 720);

		while (_running) {
			
			float time = (float)glfwGetTime();
			Timestep timestep = time - _lastFrameTime;
			_lastFrameTime = time;

			// YUICY_INFO("Timestep {}", timestep.GetSeconds());

			RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
			RenderCommand::Clear();

			// Normal Layer
			for (Layer* layer : _layerStack)
				layer->OnUpdate(timestep);

			// ImGui Layer
			_imGuiLayer->Begin();
			for (Layer* layer : _layerStack)
				layer->OnImGuiRender();
			_imGuiLayer->End();

			_window->OnUpdate();
		}
	}

	void Application::PushLayer(Layer* layer) {
		_layerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer) {
		_layerStack.PushOverlay(layer);
		layer->OnAttach();
	}
}
