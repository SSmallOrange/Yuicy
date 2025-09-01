#include <Yuicy.h>

class Sandbox : public Yuicy::Application {
public:
	Sandbox() {}
	~Sandbox() {}
};

Yuicy::Application* Yuicy::CreateApplication() {
	return new Sandbox();
}
