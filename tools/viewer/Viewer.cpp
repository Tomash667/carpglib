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
#include <BasicShader.h>

#define INCLUDE_COMMON_DIALOGS
#include <WindowsIncludes.h>

Viewer::Viewer() : scene(nullptr), camera(nullptr), lastMeshInst(nullptr), dist(1), details(false)
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

	shader = app::render->GetShader<BasicShader>();

	return true;
}

void Viewer::OnDraw()
{
	app::scene_mgr->Draw(nullptr);

	shader->Prepare(*camera);
	shader->DrawLine(Vec3(-100, 0, 0), Vec3(100, 0, 0), 0.01f, Color::Black);
	shader->DrawLine(Vec3(0, 0, -100), Vec3(0, 0, 100), 0.01f, Color::Black);
	shader->Draw();

	app::gui->Draw(true, true);
	app::render->Present();
}

void Viewer::Draw(ControlDrawData*)
{
	Rect r = { 0,0,500,500 };
	string text = Format("Fps: %g\nF1 - open mesh, F2 - toggle details", app::engine->GetFps());
	if(node->mesh)
	{
		text += Format("\nMesh: %s (v %d)", node->mesh->filename, node->mesh->head.version);
		if(node->mesh_inst)
		{
			MeshInstance::Group& group = node->mesh_inst->groups[0];
			Mesh::Animation* anim = group.anim;
			text += Format("\nAnimation: %s (%d/%u - [1] prev, [2] next)", anim ? anim->name.c_str() : "null",
				node->mesh->GetAnimationIndex(anim) + 1, node->mesh->anims.size());
			if(anim)
			{
				bool hit;
				text += Format("\nTime: %g/%g (%d/%d)", FLT10(group.time), FLT10(anim->length), group.GetFrameIndex(hit) + 1, anim->n_frames);
			}
		}
	}
	if(details)
	{
		text += Format("\nVerts: %u   Tris: %u   Subs: %u\nBones: %u   Groups: %u   Anims: %u   Points: %u",
			node->mesh->head.n_verts, node->mesh->head.n_tris, node->mesh->head.n_subs,
			node->mesh->head.n_bones, node->mesh->head.n_groups, node->mesh->head.n_anims, node->mesh->head.n_points);
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
		MeshInstance::Group& group = node->mesh_inst->groups[0];
		if(group.anim)
		{
			if(app::input->Pressed(Key::Spacebar))
			{
				if(node->mesh_inst->IsPlaying())
					node->mesh_inst->Stop();
				else
					node->mesh_inst->Play();
			}
			if(!node->mesh_inst->IsPlaying())
			{
				bool hit;
				int index = group.GetFrameIndex(hit);
				if(app::input->Pressed(Key::N3))
				{
					if(hit)
					{
						--index;
						if(index == -1)
							index = group.anim->n_frames - 1;
					}
					group.time = group.anim->frames[index].time;
					node->mesh_inst->Changed();
				}
				else if(app::input->Pressed(Key::N4))
				{
					if(hit)
					{
						++index;
						if(index == group.anim->n_frames)
							index = 0;
					}
					group.time = group.anim->frames[index].time;
					node->mesh_inst->Changed();
				}
			}
		}
		node->mesh_inst->Update(dt);
		node->mesh_inst->SetupBones();
	}

	if(app::input->Pressed(Key::Num5))
		SetCamera(Free);
	if(app::input->Pressed(Key::Num1))
		SetCamera(app::input->Down(Key::Control) ? Back : Front);
	if(app::input->Pressed(Key::Num3))
		SetCamera(app::input->Down(Key::Control) ? Right : Left);
	if(app::input->Pressed(Key::Num7))
		SetCamera(app::input->Down(Key::Control) ? Bottom : Top);

	float wheel = app::input->GetMouseWheel();
	if(wheel != 0)
	{
		dist = Clamp(dist - wheel, 0.1f, 10.f);
		UpdateCamera();
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
				if(mesh->IsAnimated() && !mesh->IsStatic())
				{
					MeshInstance* meshInst = new MeshInstance(mesh);
					node->SetMesh(meshInst);
				}
				else
					node->SetMesh(mesh);
				lastMeshInst = node->mesh_inst;

				SetCamera(Free);

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

	if(app::input->Pressed(Key::F2))
		details = !details;

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

void Viewer::SetCamera(CamMode mode)
{
	this->mode = mode;
	UpdateCamera();
}

void Viewer::UpdateCamera()
{
	if(!node->mesh)
		return;

	Mesh* mesh = node->mesh;

	switch(mode)
	{
	case Free:
		camera->to = mesh->head.cam_target;
		camera->from = mesh->head.cam_pos;
		break;
	case Front:
		camera->to = Vec3::Zero;
		camera->from = Vec3(0, 0, -mesh->head.radius * dist);
		break;
	case Back:
		camera->to = Vec3::Zero;
		camera->from = Vec3(0, 0, mesh->head.radius * dist);
		break;
	case Left:
		camera->to = Vec3::Zero;
		camera->from = Vec3(mesh->head.radius * dist, 0, 0);
		break;
	case Right:
		camera->to = Vec3::Zero;
		camera->from = Vec3(-mesh->head.radius * dist, 0, 0);
		break;
	case Bottom:
		camera->to = Vec3(0, 0, 0.01f);
		camera->from = Vec3(0, -mesh->head.radius * dist, 0);
		break;
	case Top:
		camera->to = Vec3(0, 0, 0.01f);
		camera->from = Vec3(0, mesh->head.radius * dist, 0);
		break;
	}

	Matrix matView = Matrix::CreateLookAt(camera->from, camera->to);
	Matrix matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, app::engine->GetWindowAspect(), camera->znear, camera->zfar);
	camera->mat_view_proj = matView * matProj;
}
