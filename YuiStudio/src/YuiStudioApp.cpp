#include "pch.h"

#include "Yuicy.h"
#include "Yuicy/Core/EntryPoint.h"

#include "EditorLayer.h"

class YuiStudio : public Yuicy::Application
{
public:
	YuiStudio()
		: Application(Yuicy::WindowProps("YuiStudio", 1600, 900))
	{
		PushLayer(new Yuicy::EditorLayer());
	}
};

Yuicy::Application* Yuicy::CreateApplication()
{
	return new YuiStudio();
}
