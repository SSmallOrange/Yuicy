#pragma once

#ifdef PLATFORM_WINDOWS

extern Yuicy::Application* Yuicy::CreateApplication();

int main(int argc, char** argv) {
	::Yuicy::Log::Init();

	auto app = Yuicy::CreateApplication();
	app->Run();
	delete app;
}

#endif
