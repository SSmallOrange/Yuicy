#include <Yuicy.h>
#include <Yuicy/Core/EntryPoint.h>

#include "Layer/GameLayer.h"

class TinyDungeonApp : public Yuicy::Application
{
public:
	TinyDungeonApp()
	{
		PushLayer(new TinyDungeon::GameLayer());
	}

	~TinyDungeonApp() = default;
};

Yuicy::Application* Yuicy::CreateApplication()
{
	return new TinyDungeonApp();
}