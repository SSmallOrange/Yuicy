#include "pch.h"

#include "Yuicy/Core/Application.h"
#include "Yuicy/Events/ApplicationEvent.h"

#include "Yuicy/Renderer/Renderer.h"
#include <glfw/glfw3.h>

#include <glad/glad.h>

namespace Yuicy {
	Application* Application::_instance = nullptr;

	Application::Application() 
	{ 
		YUICY_PROFILE_FUNCTION();
		YUICY_ASSERT(!_instance, "Application already exists!");
		_instance = this;

		_window = Window::Create(WindowProps());
		_window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

		Renderer::Init();

		_imGuiLayer = new ImGuiLayer();
		PushOverlay(_imGuiLayer);
	}

	Application::~Application() 
	{
		YUICY_PROFILE_FUNCTION();

	}

	void Application::OnEvent(Event& e) {
		YUICY_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1));

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
		YUICY_PROFILE_FUNCTION();
		while (_running) {
			
			YUICY_PROFILE_SCOPE("RunLoop");

			float time = (float)glfwGetTime();
			Timestep timestep = time - _lastFrameTime;
			_lastFrameTime = time;

			// YUICY_INFO("Timestep {}", timestep.GetSeconds());

			RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1 });
			RenderCommand::Clear();

			// Normal Layer
			if (!_minimized)  // 窗口最小化时停止更新
			{
				{
					YUICY_PROFILE_SCOPE("LayerStack OnUpdate");
					for (Layer* layer : _layerStack)
						layer->OnUpdate(timestep);
				}

				_imGuiLayer->Begin();
				{
					YUICY_PROFILE_SCOPE("LayerStack OnImGuiRender");
					// ImGui Layer
					for (Layer* layer : _layerStack)
						layer->OnImGuiRender();
				}
				_imGuiLayer->End();
			}

			_window->OnUpdate();
		}
	}

	void Application::PushLayer(Layer* layer) {
		YUICY_PROFILE_FUNCTION();

		_layerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* layer) {
		YUICY_PROFILE_FUNCTION();

		_layerStack.PushOverlay(layer);
		layer->OnAttach();
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		YUICY_PROFILE_FUNCTION();

		if (e.GetWidth() == 0 || e.GetHeight() == 0)
		{
			_minimized = true;
			return false;
		}

		_minimized = false;
		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

}
