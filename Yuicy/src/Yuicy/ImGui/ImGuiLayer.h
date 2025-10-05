#pragma once

#include "Yuicy/Core/Layer.h"

#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Events/KeyEvent.h"
#include "Yuicy/Events/MouseEvent.h"

namespace Yuicy {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;

		void Begin();
		void End();

		void BlockEvents(bool block) { m_BlockEvents = block; }
		
		void SetDarkThemeColors();

		uint32_t GetActiveWidgetID() const;
	private:
		bool m_BlockEvents = true;
	};

}
