#pragma once

#include "Yuicy/Core/Layer.h"

#include "Yuicy/Events/ApplicationEvent.h"
#include "Yuicy/Events/KeyEvent.h"
#include "Yuicy/Events/MouseEvent.h"

namespace Yuicy {

	class YUICY_API ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnEvent(Event& e) override;
		virtual void OnUpdate() override;

// 		void Begin();
// 		void End();

		void BlockEvents(bool block) { _blockEvents = block; }
		
 		void SetDarkThemeColors();
// 
// 		uint32_t GetActiveWidgetID() const;
	private:
		bool _blockEvents = true;
		float _time = 0.0f;
	};

}
