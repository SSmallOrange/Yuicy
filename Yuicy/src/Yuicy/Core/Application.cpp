#include "pch.h"

#include "Yuicy/Core/Application.h"
#include "Yuicy/Events/ApplicationEvent.h"

#include <glad/glad.h>

namespace Yuicy {
	Application* Application::_instance = nullptr;

	Application::Application() { 
		YUICY_PROFILE_FUNCTION();
		YUICY_ASSERT(!_instance, "Application already exists!");
		_instance = this;

		_window = Window::Create(WindowProps());
		_window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

		_imGuiLayer = new ImGuiLayer();
		PushOverlay(_imGuiLayer);

		glGenVertexArrays(1, &vertexArray);
		glBindVertexArray(vertexArray);

		float vertics[3 * 3] = {  // 坐标
			-0.5f, -0.5f, 0.0f,
			0.5f, -0.5f, 0.0f,
			0.0f, 0.5f, 0.0f,
		};

		_vertexBuffer.reset(VertexBuffer::Create(vertics, sizeof(vertics)));

		glEnableVertexAttribArray(0);  // 启用着色器  0索引
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

		uint32_t indices[3] = { 0, 1, 2 };

		_indexBuffer.reset(IndexBuffer::Create(indices, sizeof(indices)));

		std::string vertexSrc = R"(
			#version 330 core
			layout(location = 0) in vec3 a_Position;

			out vec3 v_Position;

			void main()
			{
				v_Position = a_Position;
				gl_Position = vec4(a_Position, 1.0);
			}
		)";

		std::string fragmentSrc = R"(
			#version 330 core
			layout(location = 0) out vec4 color;
			in vec3 v_Position;

			void main()
			{
				color = vec4(v_Position * 0.5 + 0.5, 1.0);
			}
		)";

		_shader = std::make_unique<Shader>(vertexSrc, fragmentSrc);
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
			
			glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			_shader->Bind();
			glBindVertexArray(vertexArray);
			glDrawElements(GL_TRIANGLES, _indexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);

			// Normal Layer
			for (Layer* layer : _layerStack)
				layer->OnUpdate();

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
