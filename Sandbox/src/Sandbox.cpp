#include <Yuicy.h>

class ExampleLayer : public Yuicy::Layer {
public:
	ExampleLayer() : Yuicy::Layer("Example") {}
	
	void OnUpdate() override {
		// YUICY_INFO("ExampleLayer::OnUpdate");
	}

	void OnEvent(Yuicy::Event& event) override {
		// YUICY_TRACE("ExampleLayer::OnEvent Info: {}", event.ToString());
		if (event.GetEventType() == Yuicy::EventType::KeyPressed)
		{
			Yuicy::KeyPressedEvent& keyPressEvent = static_cast<Yuicy::KeyPressedEvent&>(event);
			YUICY_TRACE("Key {} Pressed!", (char)keyPressEvent.GetKeyCode());
		}

	}
};

class Sandbox : public Yuicy::Application {
public:
	Sandbox() {
		PushLayer(new ExampleLayer);
	}
	~Sandbox() = default;
};

Yuicy::Application* Yuicy::CreateApplication() {

	YUICY_CORE_INFO("Hello World!!{}", 2);
	YUICY_CORE_ERROR("Bad !!{}", "World");

	return new Sandbox();
}
