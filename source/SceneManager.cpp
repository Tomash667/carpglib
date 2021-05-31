#include "Pch.h"
#include "SceneManager.h"

#include "Camera.h"
#include "DirectX.h"
#include "Gui.h"
#include "Render.h"
#include "Scene.h"
#include "SceneCallback.h"
#include "SceneNode.h"
#include "SkyboxShader.h"
#include "SuperShader.h"

SceneManager* app::scene_mgr;

//=================================================================================================
SceneManager::SceneManager() : use_lighting(true), use_fog(true), use_normalmap(true), use_specularmap(true)
{
}

//=================================================================================================
void SceneManager::Init()
{
	super_shader = app::render->GetShader<SuperShader>();
	skybox_shader = app::render->GetShader<SkyboxShader>();
}

//=================================================================================================
void SceneManager::SetScene(Scene* scene, Camera* camera)
{
	this->scene = scene;
	this->camera = camera;
	if(scene && camera)
		super_shader->SetScene(scene, camera);
}

//=================================================================================================
void SceneManager::Prepare()
{
	batch.Clear();
	batch.camera = camera;
	batch.gather_lights = use_lighting && !scene->use_light_dir;
	scene->ListNodes(batch);
	batch.Process();

	app::render->Clear(scene->clear_color);
}

//=================================================================================================
void SceneManager::Draw()
{
	if(scene && camera)
		Draw(nullptr);
	else
		app::render->Clear(Color::Black);
	app::gui->Draw();
	app::render->Present();
}

//=================================================================================================
void SceneManager::Draw(RenderTarget* target)
{
	batch.Clear();
	batch.camera = camera;
	batch.gather_lights = use_lighting && !scene->use_light_dir;
	scene->ListNodes(batch);
	batch.Process();

	if(target)
		app::render->SetRenderTarget(target);

	app::render->Clear(scene->clear_color);

	if(scene->skybox)
		skybox_shader->Draw(*scene->skybox, *camera);

	super_shader->Prepare();

	if(!batch.node_groups.empty())
		DrawSceneNodes(batch.nodes, batch.node_groups);

	if(!batch.alpha_nodes.empty())
		DrawAlphaSceneNodes(batch.alpha_nodes);

	for(SceneCallback* callback : scene->callbacks)
		callback->Draw(*camera);

	if(target)
		app::render->SetRenderTarget(nullptr);
}

//=================================================================================================
void SceneManager::DrawSceneNodes()
{
	if(batch.node_groups.empty() && batch.alpha_nodes.empty())
		return;

	super_shader->Prepare();

	if(!batch.node_groups.empty())
		DrawSceneNodes(batch.nodes, batch.node_groups);

	if(!batch.alpha_nodes.empty())
		DrawAlphaSceneNodes(batch.alpha_nodes);
}

//=================================================================================================
void SceneManager::DrawSceneNodes(SceneBatch& batch)
{
	super_shader->Prepare();

	DrawSceneNodes(batch.nodes, batch.node_groups);
}

//=================================================================================================
void SceneManager::DrawAlphaSceneNodes(SceneBatch& batch)
{
	super_shader->Prepare();

	DrawAlphaSceneNodes(batch.alpha_nodes);
}

//=================================================================================================
void SceneManager::DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups)
{
	app::render->SetBlendState(Render::BLEND_NO);

	const bool use_fog = this->use_fog && use_lighting;

	// for each group
	for(const SceneNodeGroup& group : groups)
	{
		const bool use_lighting = this->use_lighting && !IsSet(group.flags, SceneNode::F_NO_LIGHTING);

		super_shader->SetShader(super_shader->GetShaderId(
			IsSet(group.flags, SceneNode::F_HAVE_WEIGHTS),
			IsSet(group.flags, SceneNode::F_HAVE_TANGENTS),
			IsSet(group.flags, SceneNode::F_ANIMATED),
			use_fog,
			IsSet(group.flags, SceneNode::F_SPECULAR_MAP),
			IsSet(group.flags, SceneNode::F_NORMAL_MAP),
			use_lighting && !scene->use_light_dir,
			use_lighting && scene->use_light_dir));

		app::render->SetDepthState(IsSet(group.flags, SceneNode::F_NO_ZWRITE) ? Render::DEPTH_READ : Render::DEPTH_YES);
		app::render->SetRasterState(IsSet(group.flags, SceneNode::F_NO_CULLING) ? Render::RASTER_NO_CULLING : Render::RASTER_NORMAL);

		// for each node in group
		for(auto it = nodes.begin() + group.start, end = nodes.begin() + group.end + 1; it != end; ++it)
			super_shader->Draw(*it);
	}
}

//=================================================================================================
void SceneManager::DrawAlphaSceneNodes(const vector<SceneNode*>& nodes)
{
	app::render->SetBlendState(Render::BLEND_ADD_ONE);

	const bool use_fog = this->use_fog && use_lighting;

	uint last_id = -1;
	for(SceneNode* node : nodes)
	{
		const bool use_lighting = this->use_lighting && !IsSet(node->flags, SceneNode::F_NO_LIGHTING);

		uint id = super_shader->GetShaderId(
			IsSet(node->flags, SceneNode::F_HAVE_WEIGHTS),
			IsSet(node->flags, SceneNode::F_HAVE_TANGENTS),
			IsSet(node->flags, SceneNode::F_ANIMATED),
			use_fog,
			IsSet(node->flags, SceneNode::F_SPECULAR_MAP),
			IsSet(node->flags, SceneNode::F_NORMAL_MAP),
			use_lighting && !scene->use_light_dir,
			use_lighting && scene->use_light_dir);
		if(id != last_id)
		{
			app::render->SetDepthState(IsSet(node->flags, SceneNode::F_NO_ZWRITE) ? Render::DEPTH_READ : Render::DEPTH_YES);
			app::render->SetRasterState(IsSet(node->flags, SceneNode::F_NO_CULLING) ? Render::RASTER_NO_CULLING : Render::RASTER_NORMAL);

			super_shader->SetShader(id);
			last_id = id;
		}

		super_shader->Draw(node);
	}
}

//=================================================================================================
void SceneManager::DrawSkybox(Mesh* mesh)
{
	assert(mesh);
	skybox_shader->Draw(*mesh, *camera);
}
