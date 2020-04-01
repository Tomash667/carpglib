#include <CarpgLib.h>
#include "Game.h"
#include <Input.h>
#include <Engine.h>
#include <DirectX.h>
#include <Render.h>
#include <SceneManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Camera.h>
#include <ResourceManager.h>

Game::~Game()
{
	delete scene;
	delete camera;
}

bool Game::OnInit()
{
	scene = new Scene;
	camera = new Camera;
	camera->from = Vec3(-1, 3, -4);
	camera->to = Vec3::Zero;

	node = SceneNode::Get();
	node->SetMesh(app::res_mgr->Load<Mesh>("box.qmsh"));
	node->type = SceneNode::NORMAL;
	node->subs = -1;
	node->tex_override = nullptr;
	node->center = Vec3::Zero;
	node->tint = Vec4::One;
	node->mat = Matrix::IdentityMatrix;
	scene->Add(node);

	rot = 0;

	return true;
}

void Game::OnUpdate(float dt)
{
	if(app::input->Down(Key::Escape))
		app::engine->Shutdown();

	rot += dt;
	node->mat = Matrix::RotationY(rot);
}

void Game::OnDraw()
{
	app::render->Clear(Vec4(1, 0, 0, 1));
	app::scene_mgr->Draw(scene, camera, nullptr);
	app::render->Present();
}
