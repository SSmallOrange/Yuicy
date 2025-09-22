#include <Yuicy.h>

class Sandbox : public Yuicy::Application {
public:
	Sandbox() = default;
	~Sandbox() = default;
};

Yuicy::Application* Yuicy::CreateApplication() {

	YUICY_CORE_INFO("Hello World!!{}", 2);
	YUICY_CORE_ERROR("Bad !!{}", "World");

	return new Sandbox();
}
