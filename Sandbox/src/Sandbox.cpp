#include <Yuicy.h>

class Sandbox : public Yuicy::Application {
public:
	Sandbox() = default;
	~Sandbox() = default;
};

Yuicy::Application* Yuicy::CreateApplication() {

	HZ_CORE_INFO(u8"Hello World!!{}", 2);
	HZ_CORE_ERROR(u8"Bad !!{}", u8"World");

	return new Sandbox();
}
