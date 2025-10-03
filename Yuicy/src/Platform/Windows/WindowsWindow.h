#pragma once

#include "Yuicy/Core/Window.h"

struct GLFWwindow;

namespace Yuicy {

	class WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();

		void OnUpdate() override;  // 逐帧更新

		unsigned int GetWidth() const override { return _Data.Width; }
		unsigned int GetHeight() const override { return _Data.Height; }

		// Window attributes
		void SetEventCallback(const EventCallbackFn& callback) override { _Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		virtual void* GetNativeWindow() const override { return _Window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
	private:
		GLFWwindow* _Window;
		// Scope<GraphicsContext> _Context;

		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			bool VSync;

			EventCallbackFn EventCallback;
		};

		WindowData _Data;
	};

}