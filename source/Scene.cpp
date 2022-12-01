#include "Pch.h"
#include "Scene.h"

#include "Algorithm.h"
#include "Camera.h"
#include "SceneManager.h"
#include "SceneNode.h"

//=================================================================================================
Scene::Scene() : clearColor(Color::Black), ambientColor(0.4f, 0.4f, 0.4f), lightColor(Color::White), fogColor(Color::Gray), useLightDir(false),
fogRange(50, 100)
{
}

//=================================================================================================
Scene::~Scene()
{
	SceneNode::Free(nodes);
}

//=================================================================================================
void Scene::ListNodes(SceneBatch& batch)
{
	FrustumPlanes frustum(batch.camera->matViewProj);
	for(SceneNode* node : nodes)
	{
		if(node->mesh && frustum.SphereToFrustum(node->center, node->radius))
		{
			if(batch.gatherLights)
				GatherLights(batch, node);
			batch.Add(node);
		}
	}
}

//=================================================================================================
void Scene::GatherLights(SceneBatch& batch, SceneNode* node)
{
	TopN<Light*, 3, float, std::less<>> best(nullptr, batch.camera->zfar);

	for(Light& light : lights)
	{
		float dist = Vec3::Distance(node->center, light.pos);
		if(dist < light.range + node->radius)
			best.Add(&light, dist);
	}

	for(int i = 0; i < 3; ++i)
		node->lights[i] = best[i];
}

//=================================================================================================
Vec4 Scene::GetAmbientColor() const
{
	if(app::sceneMgr->useLighting)
		return ambientColor;
	return Vec4::One;
}

//=================================================================================================
Vec4 Scene::GetFogParams() const
{
	if(app::sceneMgr->useLighting && app::sceneMgr->useFog)
		return Vec4(fogRange.x, fogRange.y, fogRange.y - fogRange.x, 1);
	return Vec4(999, 1000, 1, 0);
}
