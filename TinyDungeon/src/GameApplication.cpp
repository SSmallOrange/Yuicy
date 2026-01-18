#include <Yuicy.h>
#include <Yuicy/Core/EntryPoint.h>

#include "Layer/GameLayer.h"
#include "Layer/UILayer.h"

class TinyDungeonApp : public Yuicy::Application
{
public:
	TinyDungeonApp()
		: Yuicy::Application(Yuicy::WindowProps("TinyDungeon", 960, 576, true))
	{
		PushLayer(new TinyDungeon::GameLayer());
		PushLayer(new TinyDungeon::UILayer());

		// Cursor
		GetWindow().SetCursor("assets/textures/cursor.png", 11, 9);
	}

	~TinyDungeonApp() = default;
};

Yuicy::Application* Yuicy::CreateApplication()
{
	return new TinyDungeonApp();
}