#include "Pch.h"
#include "App.h"

#include "Engine.h"

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
