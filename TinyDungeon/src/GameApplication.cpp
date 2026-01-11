#include <Yuicy.h>
#include <Yuicy/Core/EntryPoint.h>

#include "Layer/GameLayer.h"

class TinyDungeonApp : public Yuicy::Application
{
public:
	TinyDungeonApp()
		: Yuicy::Application(Yuicy::WindowProps("TinyDungeon", 960, 576))
	{
		PushLayer(new TinyDungeon::GameLayer());
	}

	~TinyDungeonApp() = default;
};

Yuicy::Application* Yuicy::CreateApplication()
{
	return new TinyDungeonApp();
}