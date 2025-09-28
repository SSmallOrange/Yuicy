#include "pch.h"
#include "Yuicy/Core/Application.h"

#include "Yuicy/Events/ApplicationEvent.h"

namespace Yuicy {
	Application::Application() { 
		_window = Window::Create(WindowProps());
	}

	Application::~Application() {

	}

	void Application::Run() {

		WindowResizeEvent e(1280, 720);
		YUICY_TRACE("{}", e.ToString());

		while (_running) {
			_window->OnUpdate();
		}
	}
}
