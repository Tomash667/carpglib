#include "Pch.h"
#include "App.h"

#include "Engine.h"
#include "Gui.h"
#include "Input.h"
#include "Render.h"
#include "SceneManager.h"

//-----------------------------------------------------------------------------
App* app::app;

//=================================================================================================
App::App()
{
	Logger::SetInstance(new PreLogger());
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
	app::sceneMgr->Draw();
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
