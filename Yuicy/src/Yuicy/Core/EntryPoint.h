#pragma once

#ifdef PLATFORM_WINDOWS

#include "Yuicy/Debug/Instrumentor.h"

extern Yuicy::Application* Yuicy::CreateApplication();

int main(int argc, char** argv) {
	::Yuicy::Log::Init();

	YUICY_PROFILE_BEGIN_SESSION("Startup", "Profiles-Startup.json");
	auto app = Yuicy::CreateApplication();
	YUICY_PROFILE_END_SESSION();

	YUICY_PROFILE_BEGIN_SESSION("RunTime", "Profiles-RunTime.json");
	app->Run();
	YUICY_PROFILE_END_SESSION();

	YUICY_PROFILE_BEGIN_SESSION("DeleteTime", "Profiles-DeleteTime.json");
	delete app;
	YUICY_PROFILE_END_SESSION();
}

#endif
