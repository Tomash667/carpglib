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
	app::res_mgr->AddDir("data");
	app::render->SetShadersDir("../shaders");

	scene = new Scene;
	scene->clear_color = Color::Red;

	camera = new Camera;
	camera->from = Vec3(-1, 3, -4);
	camera->to = Vec3::Zero;
	Matrix matView = Matrix::CreateLookAt(camera->from, camera->to);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), 0.1f, 50.f);
	camera->mat_view_proj = matView * matProj;

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
	app::scene_mgr->Draw(scene, camera, nullptr);
	app::render->Present();
}
