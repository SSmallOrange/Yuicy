#pragma once

#include "Yuicy/Core/Window.h"

struct GLFWwindow;
struct GLFWcursor;

namespace Yuicy {

	class GraphicsContext;

	// 纯 GLFW 实现，所有平台都编译。始终使用系统标题栏，忽略 WindowProps::borderlessWindow
	class GlfwWindow : public Window
	{
	public:
		GlfwWindow(const WindowProps& props);
		virtual ~GlfwWindow();

		void OnUpdate() override;

		uint32_t GetWidth() const override { return m_Data.Width; }
		uint32_t GetHeight() const override { return m_Data.Height; }
		uint32_t GetFramebufferWidth() const override { return m_Data.FramebufferWidth; }
		uint32_t GetFramebufferHeight() const override { return m_Data.FramebufferHeight; }

		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		void* GetNativeWindow() const override { return m_Window; }

		void SetCursor(const std::string& imagePath, int hotspotX = 0, int hotspotY = 0) override;
		void ResetCursor() override;
		void SetCursorVisible(bool visible) override;

		void Close() override;
		void CancelClose() override;
		void Minimize() override;
		void Maximize() override;
		void Restore() override;
		bool IsMaximized() const override;

	private:
		void Init(const WindowProps& props);
		void Shutdown();

	private:
		GLFWwindow* m_Window = nullptr;
		GLFWcursor* m_CustomCursor = nullptr;
		Scope<GraphicsContext> m_Context;

		// 通过 glfwSetWindowUserPointer 交给 GLFW 回调使用
		struct WindowData
		{
			std::string Title;
			uint32_t Width = 0;
			uint32_t Height = 0;
			uint32_t FramebufferWidth = 0;
			uint32_t FramebufferHeight = 0;
			bool VSync = false;

			EventCallbackFn EventCallback;
		};

		WindowData m_Data;
	};

}
