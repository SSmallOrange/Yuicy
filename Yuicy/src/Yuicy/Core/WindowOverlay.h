#pragma once

#include "Yuicy/Core/Base.h"
#include "Yuicy/Renderer/Texture.h"

#include <string>
#include <glm/glm.hpp>

namespace Yuicy {

	struct WindowOverlayConfig
	{
		// 标题栏高度
		float titleBarHeight = 32.0f;

		// 按钮大小
		float buttonSize = 20.0f;
		float buttonPadding = 8.0f;

		// 按钮纹理（可选，如果为null则回退到文本）
		std::string closeButtonTexture = "";
		std::string minimizeButtonTexture = "";
		std::string maximizeButtonTexture = "";

		// 颜色
		glm::vec4 titleBarColor = { 0.15f, 0.15f, 0.18f, 0.5f };
		glm::vec4 buttonHoverColor = { 0.3f, 0.3f, 0.35f, 1.0f };
		glm::vec4 closeButtonHoverColor = { 0.8f, 0.2f, 0.2f, 1.0f };

		// Optional title
		std::string windowTitle = "";
		bool showTitle = false;

		// Enable/disable features
		bool enableDragging = true;
		bool showMinimizeButton = true;
		bool showMaximizeButton = true;
		bool showCloseButton = true;
	};

	class WindowOverlay
	{
	public:
		virtual ~WindowOverlay() = default;

		virtual void Init() = 0;
		virtual void Shutdown() = 0;

		virtual void OnImGuiRender() = 0;

		virtual void SetConfig(const WindowOverlayConfig& config) = 0;
		virtual WindowOverlayConfig& GetConfig() = 0;
		virtual const WindowOverlayConfig& GetConfig() const = 0;

		virtual void SetEnabled(bool enabled) = 0;
		virtual bool IsEnabled() const = 0;

		static Scope<WindowOverlay> Create();
	};

}
