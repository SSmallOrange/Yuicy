#pragma once

#include "Yuicy/Core/WindowOverlay.h"
#include "Yuicy/Renderer/Texture.h"

struct GLFWwindow;

namespace Yuicy {

	class WindowsWindowOverlay : public WindowOverlay
	{
	public:
		WindowsWindowOverlay();
		~WindowsWindowOverlay() override;

		void Init() override;
		void Shutdown() override;

		void OnImGuiRender() override;

		void SetConfig(const WindowOverlayConfig& config) override { m_config = config; }
		WindowOverlayConfig& GetConfig() override { return m_config; }
		const WindowOverlayConfig& GetConfig() const override { return m_config; }

		void SetEnabled(bool enabled) override { m_enabled = enabled; }
		bool IsEnabled() const override { return m_enabled; }

	private:
		void RenderTitleBar();
		void HandleWindowDragging();

	private:
		WindowOverlayConfig m_config;
		bool m_enabled = true;
		bool m_initialized = false;

		// 按钮纹理
		Ref<Texture2D> m_closeButtonTex;
		Ref<Texture2D> m_minimizeButtonTex;
		Ref<Texture2D> m_maximizeButtonTex;

		// 拖拽状态
		bool m_isDragging = false;
		glm::vec2 m_dragStartPos = { 0.0f, 0.0f };
		glm::vec2 m_windowStartPos = { 0.0f, 0.0f };

		GLFWwindow* m_window = nullptr;
	};

}
