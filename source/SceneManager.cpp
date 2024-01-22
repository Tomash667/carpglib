#include "Pch.h"
#include "SceneManager.h"

#include "App.h"
#include "Camera.h"
#include "DirectX.h"
#include "Gui.h"
#include "ParticleShader.h"
#include "Render.h"
#include "Scene.h"
#include "SceneNode.h"
#include "SuperShader.h"

SceneManager* app::sceneMgr;

//=================================================================================================
SceneManager::SceneManager() : scene(nullptr), camera(nullptr), useLighting(true), useFog(true), useNormalmap(true), useSpecularmap(true)
{
}

//=================================================================================================
void SceneManager::Init()
{
	superShader = app::render->GetShader<SuperShader>();
	particleShader = app::render->GetShader<ParticleShader>();
}

//=================================================================================================
void SceneManager::SetScene(Scene* scene, Camera* camera)
{
	this->scene = scene;
	this->camera = camera;
	if(scene && camera)
		superShader->SetScene(scene, camera);
}

//=================================================================================================
void SceneManager::ListNodes()
{
	batch.Clear();
	batch.camera = camera;
	batch.gatherLights = useLighting && !scene->useLightDir;
	scene->ListNodes(batch);
	batch.Process();
}

//=================================================================================================
void SceneManager::Draw()
{
	if(!scene || !camera)
	{
		app::render->Clear(Color::Black);
		app::render->Present();
		return;
	}

	app::render->Clear(scene->clearColor);

	ListNodes();

	if(!batch.nodeGroups.empty() || !batch.alphaNodes.empty())
	{
		superShader->Prepare();
		superShader->SetScene(scene, camera);

		if(!batch.nodeGroups.empty())
			DrawSceneNodes(batch.nodes, batch.nodeGroups);

		if(!batch.alphaNodes.empty())
			DrawAlphaSceneNodes(batch.alphaNodes);
	}

	if(!batch.particleEmitters.empty())
	{
		particleShader->Prepare(*camera);
		particleShader->DrawParticles(batch.particleEmitters);
	}

	app::app->OnCustomDraw();
	app::gui->Draw();
	app::render->Present();
}

//=================================================================================================
void SceneManager::DrawSceneNodes(SceneBatch& batch)
{
	superShader->Prepare();

	DrawSceneNodes(batch.nodes, batch.nodeGroups);
}

//=================================================================================================
void SceneManager::DrawAlphaSceneNodes(SceneBatch& batch)
{
	superShader->Prepare();

	DrawAlphaSceneNodes(batch.alphaNodes);
}

//=================================================================================================
void SceneManager::DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups)
{
	app::render->SetBlendState(Render::BLEND_NO);

	const bool useFog = this->useFog && useLighting;

	// for each group
	for(const SceneNodeGroup& group : groups)
	{
		const bool useLighting = this->useLighting && !IsSet(group.flags, SceneNode::F_NO_LIGHTING);

		superShader->SetShader(superShader->GetShaderId(
			IsSet(group.flags, SceneNode::F_HAVE_WEIGHTS),
			IsSet(group.flags, SceneNode::F_HAVE_TANGENTS),
			IsSet(group.flags, SceneNode::F_ANIMATED),
			useFog,
			IsSet(group.flags, SceneNode::F_SPECULAR_MAP),
			IsSet(group.flags, SceneNode::F_NORMAL_MAP),
			useLighting && !scene->useLightDir,
			useLighting && scene->useLightDir));

		app::render->SetDepthState(IsSet(group.flags, SceneNode::F_NO_ZWRITE) ? Render::DEPTH_READ : Render::DEPTH_YES);
		app::render->SetRasterState(IsSet(group.flags, SceneNode::F_NO_CULLING) ? Render::RASTER_NO_CULLING : Render::RASTER_NORMAL);

		// for each node in group
		for(auto it = nodes.begin() + group.start, end = nodes.begin() + group.end + 1; it != end; ++it)
			superShader->Draw(*it);
	}
}

//=================================================================================================
void SceneManager::DrawAlphaSceneNodes(const vector<SceneNode*>& nodes)
{
	const bool useFog = this->useFog && useLighting;

	uint last_id = -1;
	for(SceneNode* node : nodes)
	{
		const bool useLighting = this->useLighting && !IsSet(node->flags, SceneNode::F_NO_LIGHTING);

		uint id = superShader->GetShaderId(
			IsSet(node->flags, SceneNode::F_HAVE_WEIGHTS),
			IsSet(node->flags, SceneNode::F_HAVE_TANGENTS),
			IsSet(node->flags, SceneNode::F_ANIMATED),
			useFog,
			IsSet(node->flags, SceneNode::F_SPECULAR_MAP),
			IsSet(node->flags, SceneNode::F_NORMAL_MAP),
			useLighting && !scene->useLightDir,
			useLighting && scene->useLightDir);
		if(id != last_id)
		{
			app::render->SetDepthState(IsSet(node->flags, SceneNode::F_NO_ZWRITE) ? Render::DEPTH_READ : Render::DEPTH_YES);
			app::render->SetRasterState(IsSet(node->flags, SceneNode::F_NO_CULLING) ? Render::RASTER_NO_CULLING : Render::RASTER_NORMAL);

			superShader->SetShader(id);
			last_id = id;
		}

		app::render->SetBlendState(node->addBlend ? Render::BLEND_ADD : Render::BLEND_ADD_ONE);

		superShader->Draw(node);
	}
}

//=================================================================================================
SceneBatch& SceneManager::GetBatch()
{
	batch.Clear();
	batch.camera = camera;
	batch.gatherLights = useLighting && !scene->useLightDir;
	return batch;
}
