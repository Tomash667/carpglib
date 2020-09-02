#include "Pch.h"
#include "Scene.h"

#include "Algorithm.h"
#include "Camera.h"
#include "Light.h"
#include "SceneManager.h"
#include "SceneNode.h"

//=================================================================================================
Scene::Scene() : clear_color(Color::Black), ambient_color(0.4f, 0.4f, 0.4f), light_color(Color::White), fog_color(Color::Gray), use_light_dir(false),
fog_range(50, 100)
{
}

//=================================================================================================
void Scene::ListNodes(SceneBatch& batch, vector<SceneNode*>& nodes)
{
	for(SceneNode* node : nodes)
	{
		if(!node->visible)
			continue;
		if(node->mesh && batch.frustum.SphereToFrustum(node->center, node->radius))
		{
			if(node->mesh_inst)
				node->mesh_inst->SetupBones();
			if(batch.gather_lights && !IsSet(node->flags, SceneNode::F_NO_LIGHTING))
				GatherLights(batch, node);
			batch.Add(node);
		}
		for(SceneNode* child : node->childs)
		{
			if(child->mesh && batch.frustum.SphereToFrustum(child->center, child->radius))
			{
				if(child->mesh_inst)
					child->mesh_inst->SetupBones();
				if(batch.gather_lights && !IsSet(child->flags, SceneNode::F_NO_LIGHTING))
					GatherLights(batch, child);
				batch.Add(child);
			}
		}
	}
}

//=================================================================================================
void Scene::GatherLights(SceneBatch& batch, SceneNode* node)
{
	TopN<Light*, 3, float, std::less<>> best(nullptr, batch.camera->zfar);

	for(Light* light : lights)
	{
		float dist = Vec3::Distance(node->center, light->pos);
		if(dist < light->range + node->radius)
			best.Add(light, dist);
	}

	for(int i = 0; i < 3; ++i)
		node->lights[i] = best[i];
}

//=================================================================================================
Vec4 Scene::GetAmbientColor() const
{
	if(app::scene_mgr->use_lighting)
		return ambient_color;
	return Vec4::One;
}

//=================================================================================================
Vec4 Scene::GetFogParams() const
{
	if(app::scene_mgr->use_lighting && app::scene_mgr->use_fog)
		return Vec4(fog_range.x, fog_range.y, fog_range.y - fog_range.x, 1);
	return Vec4(999, 1000, 1, 0);
}
