#include "pch.h"

#include "Yuicy/Core/Application.h"
#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Scripting/LuaScriptEngine.h"
#include "Yuicy/Project/Project.h"

#include "Yuicy/Renderer/Renderer.h"

namespace Yuicy {
	Application* Application::_instance = nullptr;

	Application::Application(const WindowProps& props) 
	{ 
		YUICY_PROFILE_FUNCTION();
		YUICY_ASSERT(!_instance, "Application already exists!");
		_instance = this;

		_window = Window::Create(props);
		_window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

		Renderer::Init();
		LuaScriptEngine::Init();

		_imGuiLayer = new ImGuiLayer();
		PushOverlay(_imGuiLayer);
	}

	Application::~Application() 
	{
		YUICY_PROFILE_FUNCTION();

		LuaScriptEngine::Shutdown();

		// 静态对象持有的 GPU 资源必须在 _window 析构（销毁图形上下文）之前释放
		Project::SetActive(nullptr);
		Renderer::Shutdown();
	}

	void Application::OnEvent(Event& e) {
		YUICY_PROFILE_FUNCTION();

		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResize, this, std::placeholders::_1));
		dispatcher.Dispatch<WindowFramebufferResizeEvent>(std::bind(&Application::OnWindowFramebufferResize, this, std::placeholders::_1));

		for (auto it = _layerStack.rbegin(); it != _layerStack.rend(); ++it) {  // 事件反向冒泡
			if (e.Handled)
				break;
			(*it)->OnEvent(e);
		}

		// WindowCloseEvent：仅在没有被 Layer 拦截时关闭
		if (e.GetEventType() == EventType::WindowClose && !e.Handled)
			OnWindowClose(e);

		// YUICY_CORE_INFO("EventInfo:{}", e.ToString());
	}

	bool Application::OnWindowClose(Event& e) {
		_running = false;
		return true;
	}

	void Application::Run() {

		WindowResizeEvent e(1280, 720);
		YUICY_PROFILE_FUNCTION();
		// 从进入主循环开始计时，首帧的 Timestep 不包含窗口创建与 Layer 初始化的耗时
		_lastFrameTime = std::chrono::steady_clock::now();
		while (_running) {
			
			YUICY_PROFILE_SCOPE("RunLoop");

			std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
			Timestep timestep = std::chrono::duration<float>(time - _lastFrameTime).count();
			_lastFrameTime = time;

			// YUICY_INFO("Timestep {}", timestep.GetSeconds());

			ExecuteMainThreadQueue();

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

	void Application::SubmitToMainThread(std::function<void()> task)
	{
		std::scoped_lock lock(_mainThreadQueueMutex);
		_mainThreadQueue.emplace_back(std::move(task));
	}

	void Application::ExecuteMainThreadQueue()
	{
		YUICY_PROFILE_SCOPE("MainThreadQueue");
		// 防止任务内部再次调用 SubmitToMainThread 导致死锁，新任务会在下一帧执行
		std::vector<std::function<void()>> tasks;
		{
			std::scoped_lock lock(_mainThreadQueueMutex);
			tasks.swap(_mainThreadQueue);
		}

		for (auto& task : tasks)
			task();
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
		// 视口按像素设置，不能用事件里的窗口坐标尺寸
		// TODO: 改为只由 OnWindowFramebufferResize 设置视口（WindowsWindow 发送 WindowFramebufferResizeEvent 后）
		Renderer::OnWindowResize(_window->GetFramebufferWidth(), _window->GetFramebufferHeight());

		return false;
	}

	bool Application::OnWindowFramebufferResize(WindowFramebufferResizeEvent& e)
	{
		YUICY_PROFILE_FUNCTION();

		// 尺寸为 0 表示最小化，_minimized 由 OnWindowResize 维护，这里只跳过
		if (e.GetWidth() == 0 || e.GetHeight() == 0)
			return false;

		Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());

		return false;
	}

}
