#pragma once

#include "Yuicy/Core/Core.h"
#include "Yuicy/Core/Base.h"
// #include "Yuicy/Core/Timestep.h"
#include "Yuicy/Events/Event.h"

namespace Yuicy {

	class YUICY_API Layer {
	public:
		Layer(const std::string& name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		const std::string& GetName() const { return _debugName; }
	protected:
		std::string _debugName;
	};

}