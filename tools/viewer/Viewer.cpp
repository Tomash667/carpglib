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
	app::resMgr->AddDir(dataDir.c_str());

	app::gui->Add(this);
	font = app::gui->GetFont("Arial", 14);

	scene = new Scene;
	scene->clearColor = Color(128, 128, 255);

	node = SceneNode::Get();
	node->mesh = nullptr;
	node->meshInst = nullptr;
	node->texOverride = nullptr;
	node->mat = Matrix::IdentityMatrix;
	node->tint = Vec4::One;
	node->center = Vec3::Zero;
	scene->Add(node);

	camera = new Camera;

	app::sceneMgr->SetScene(scene, camera);

	return true;
}

void Viewer::OnDraw()
{
	app::sceneMgr->Draw(nullptr);
	app::gui->Draw(true, true);
	app::render->Present();
}

void Viewer::Draw()
{
	Rect r = { 0,0,500,500 };
	string text = Format("Fps: %g\nF1 - open mesh, F2 - toggle details", app::engine->GetFps());
	if(node->mesh)
	{
		text += Format("\nMesh: %s (v %d)", node->mesh->filename, node->mesh->head.version);
		if(node->meshInst)
		{
			MeshInstance::Group& group = node->meshInst->groups[0];
			Mesh::Animation* anim = group.anim;
			text += Format("\nAnimation: %s (%d/%u - [1] prev, [2] next)", anim ? anim->name.c_str() : "null",
				node->mesh->GetAnimationIndex(anim) + 1, node->mesh->anims.size());
			if(anim)
			{
				bool hit;
				text += Format("\nTime: %g/%g (%d/%d)", FLT10(group.time), FLT10(anim->length), group.GetFrameIndex(hit) + 1, anim->nFrames);
			}
		}
	}
	if(details)
	{
		text += Format("\nVerts: %u   Tris: %u   Subs: %u\nBones: %u   Groups: %u   Anims: %u   Points: %u",
			node->mesh->head.nVerts, node->mesh->head.nTris, node->mesh->head.nSubs,
			node->mesh->head.nBones, node->mesh->head.nGroups, node->mesh->head.nAnims, node->mesh->head.nPoints);
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
	if(node->meshInst)
	{
		MeshInstance::Group& group = node->meshInst->groups[0];
		if(group.anim)
		{
			if(app::input->Pressed(Key::Spacebar))
			{
				if(node->meshInst->IsPlaying())
					node->meshInst->Stop();
				else
					node->meshInst->Play();
			}
			if(!node->meshInst->IsPlaying())
			{
				bool hit;
				int index = group.GetFrameIndex(hit);
				if(app::input->Pressed(Key::N3))
				{
					if(hit)
					{
						--index;
						if(index == -1)
							index = group.anim->nFrames - 1;
					}
					group.time = group.anim->frames[index].time;
					node->meshInst->Changed();
				}
				else if(app::input->Pressed(Key::N4))
				{
					if(hit)
					{
						++index;
						if(index == group.anim->nFrames)
							index = 0;
					}
					group.time = group.anim->frames[index].time;
					node->meshInst->Changed();
				}
			}
		}
		node->meshInst->Update(dt);
		node->meshInst->SetupBones();
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
				Mesh* mesh = app::resMgr->Load<Mesh>(filename);
				if(node->meshInst)
					delete node->meshInst;
				if(mesh->IsAnimated() && !mesh->IsStatic())
				{
					MeshInstance* meshInst = new MeshInstance(mesh);
					node->SetMesh(meshInst);
				}
				else
					node->SetMesh(mesh);
				lastMeshInst = node->meshInst;

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
		int index = node->mesh->GetAnimationIndex(node->meshInst->groups[0].anim);
		if(index != -1)
		{
			--index;
			if(index != -1)
				node->meshInst->Play(&node->mesh->anims[index], 0, 0);
			else
				node->meshInst->DisableAnimations();
		}
	}
	if(app::input->Pressed(Key::N2))
	{
		int index = node->mesh->GetAnimationIndex(node->meshInst->groups[0].anim);
		if(index + 1 != (int)node->mesh->anims.size())
		{
			++index;
			node->meshInst->Play(&node->mesh->anims[index], 0, 0);
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
		camera->to = mesh->head.camTarget;
		camera->from = mesh->head.camPos;
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

	camera->UpdateMatrix();
}
