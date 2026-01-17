#pragma once

#include "Yuicy/Events/Event.h"

#include <sstream>
#include <functional>

namespace Yuicy {

	struct WindowProps  // 窗口属性
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool borderlessWindow = true;

		WindowProps(const std::string& title = "Yuicy Engine",
			        uint32_t width = 1600,
			        uint32_t height = 900,
			        bool isBorderlessWindow = true)
			: Title(title), Width(width), Height(height), borderlessWindow(isBorderlessWindow)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;  // 垂直同步
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		// Cursor
		virtual void SetCursor(const std::string& imagePath, int hotspotX = 0, int hotspotY = 0) = 0;
		virtual void ResetCursor() = 0;
		virtual void SetCursorVisible(bool visible) = 0;

		// Window control
		virtual void Close() = 0;
		virtual void Minimize() = 0;
		virtual void Maximize() = 0;
		virtual void Restore() = 0;
		virtual bool IsMaximized() const = 0;

		static Scope<Window> Create(const WindowProps& props = WindowProps());
	};

}
