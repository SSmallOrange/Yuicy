#include "pch.h"
#include "Yuicy/Core/Application.h"

#include "Yuicy/Events/ApplicationEvent.h"

namespace Yuicy {
	Application::Application() { 
		 
	}

	Application::~Application() {

	}

	void Application::Run() {

		WindowResizeEvent e(1280, 720);
		YUICY_TRACE("{}", e.ToString());
		// YUICY_TRACE("{}", static_cast<const Yuicy::Event&>(e));
		// YUICY_TRACE("{}", e);

		while (true);
	}
}
