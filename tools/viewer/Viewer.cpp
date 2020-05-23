#include "Pch.h"
#include "Viewer.h"
#include <Engine.h>
#include <Render.h>
#include <Input.h>
#include <Scene.h>
#include <SceneManager.h>
#include <Camera.h>
#include <Config.h>
#include <ResourceManager.h>
#include <Gui.h>
#include <File.h>

#define INCLUDE_COMMON_DIALOGS
#include <WindowsIncludes.h>

Viewer::Viewer() : scene(nullptr), camera(nullptr), lastMeshInst(nullptr)
{
	app::engine->SetTitle("Mesh viewer");

	Config cfg;
	cfg.Load("resource.cfg");
	app::render->SetShadersDir(cfg.GetString("shaders", "shaders").c_str());
	dataDir = cfg.GetString("data", "data");

	Logger::SetInstance(new ConsoleLogger);
}

Viewer::~Viewer()
{
	delete scene;
	delete camera;
	delete lastMeshInst;
}

bool Viewer::OnInit()
{
	app::res_mgr->AddDir(dataDir.c_str());

	app::gui->Add(this);
	font = app::gui->GetFont("Arial", 14);

	scene = new Scene;
	scene->clear_color = Color(128, 128, 255);

	node = SceneNode::Get();
	node->mesh = nullptr;
	node->mesh_inst = nullptr;
	node->tex_override = nullptr;
	node->mat = Matrix::IdentityMatrix;
	node->tint = Vec4::One;
	node->center = Vec3::Zero;
	scene->Add(node);

	camera = new Camera;

	app::scene_mgr->SetScene(scene, camera);

	FileReader f("../../../../bin/data/meshes/items/weapon/kilof.qmsh");
	f.IsOpen();

	return true;
}

void Viewer::OnDraw()
{
	app::scene_mgr->Draw(nullptr);
	app::gui->Draw(true, true);
	app::render->Present();
}

void Viewer::Draw(ControlDrawData*)
{
	Rect r = { 0,0,500,500 };
	string text = Format("Fps: %g\nF1 - open mesh", app::engine->GetFps());
	if(node->mesh)
	{
		text += Format("\nMesh: %s (v %d)", node->mesh->filename, node->mesh->head.version);
		if(node->mesh_inst)
		{
			Mesh::Animation* anim = node->mesh_inst->groups[0].anim;
			text += Format("\nAnimation: %s (%d/%u - [1] prev, [2] next)", anim ? anim->name.c_str() : "null",
				node->mesh->GetAnimationIndex(anim) + 1, node->mesh->anims.size());
		}
	}
	gui->DrawText(font, text, 0, Color::Black, r);
}

void Viewer::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Pressed(Key::Escape))
	{
		app::engine->Shutdown();
		return;
	}
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		app::engine->SetFullscreen(!app::engine->IsFullscreen());
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		app::engine->UnlockCursor();

	if(app::input->Down(Key::Left))
		rot += dt * 3;
	if(app::input->Down(Key::Right))
		rot -= dt * 3;
	node->mat = Matrix::RotationY(rot);
	if(node->mesh_inst)
	{
		node->mesh_inst->Update(dt);
		node->mesh_inst->SetupBones();
	}

	if(app::input->Pressed(Key::F1))
	{
		char buf[MAX_PATH] = {};

		OPENFILENAME open = {};
		open.lStructSize = sizeof(OPENFILENAME);
		open.hwndOwner = app::engine->GetWindowHandle();
		open.lpstrFilter = "Qmsh file\0*.qmsh\0\0";
		open.lpstrFile = buf;
		open.nMaxFile = MAX_PATH;
		open.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		app::engine->UnlockCursor();

		if(GetOpenFileName(&open))
		{
			try
			{
				cstring filename = io::FilenameFromPath(buf);
				Mesh* mesh = app::res_mgr->Load<Mesh>(filename);
				if(node->mesh_inst)
					delete node->mesh_inst;
				if(mesh->IsAnimated())
				{
					MeshInstance* meshInst = new MeshInstance(mesh);
					node->SetMesh(meshInst);
				}
				else
					node->SetMesh(mesh);
				lastMeshInst = node->mesh_inst;

				camera->to = mesh->head.cam_target;
				camera->from = mesh->head.cam_pos;
				Matrix matView = Matrix::CreateLookAt(camera->from, camera->to);
				Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), camera->znear, camera->zfar);
				camera->mat_view_proj = matView * matProj;

				rot = 0;
				node->mat = Matrix::IdentityMatrix;
			}
			catch(cstring err)
			{
				Error("Failed to load mesh '%s': %s", buf, err);
			}
		}

		app::engine->LockCursor();
	}

	if(app::input->Pressed(Key::N1))
	{
		int index = node->mesh->GetAnimationIndex(node->mesh_inst->groups[0].anim);
		if(index != -1)
		{
			--index;
			if(index != -1)
				node->mesh_inst->Play(&node->mesh->anims[index], 0, 0);
			else
				node->mesh_inst->DisableAnimations();
		}
	}
	if(app::input->Pressed(Key::N2))
	{
		int index = node->mesh->GetAnimationIndex(node->mesh_inst->groups[0].anim);
		if(index + 1 != (int)node->mesh->anims.size())
		{
			++index;
			node->mesh_inst->Play(&node->mesh->anims[index], 0, 0);
		}
	}
}
