#include <CarpgLib.h>
#include "Game.h"
#include <Input.h>
#include <Engine.h>
#include <DirectX.h>
#include <Render.h>

void Game::OnUpdate(float dt)
{
	if(app::input->Down(Key::Escape))
		app::engine->Shutdown();
}

void Game::OnDraw()
{
	app::render->Clear(Vec4(1, 0, 0, 1));
	app::render->Present();
}
