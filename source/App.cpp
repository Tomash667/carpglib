#include "Pch.h"
#include "App.h"

#include "Engine.h"
#include "Input.h"
#include "Gui.h"
#include "Render.h"

//-----------------------------------------------------------------------------
App* app::app;

//=================================================================================================
App::App()
{
	app::app = this;
	app::engine = new Engine;
}

//=================================================================================================
App::~App()
{
	delete app::engine;
}

//=================================================================================================
bool App::Start()
{
	return app::engine->Start();
}

//=================================================================================================
void App::OnDraw()
{
	app::render->Clear(Color::Blue);
	app::render->Present();
}

//=================================================================================================
void App::OnResize()
{
	app::gui->OnResize();
}

//=================================================================================================
void App::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4))
	{
		app::engine->Shutdown();
		return;
	}
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		app::engine->ToggleFullscreen();
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		app::engine->UnlockCursor();
}
