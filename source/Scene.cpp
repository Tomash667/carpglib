#include "Pch.h"
#include "Scene.h"

#include "Algorithm.h"
#include "Camera.h"
#include "ParticleSystem.h"
#include "SceneManager.h"
#include "SceneNode.h"
#include "Terrain.h"

//=================================================================================================
Scene::Scene() : terrain(nullptr), skybox(nullptr), customMesh(customMesh), clearColor(Color::Black), ambientColor(0.4f, 0.4f, 0.4f), lightColor(Color::White),
fogColor(Color::Gray), useLightDir(false), fogRange(50, 100)
{
}

//=================================================================================================
Scene::~Scene()
{
	Clear();
}

//=================================================================================================
void Scene::Remove(SceneNode* node)
{
	assert(node);
	RemoveElement(nodes, node);
	node->Free();
}

//=================================================================================================
void Scene::Detach(SceneNode* node)
{
	assert(node);
	RemoveElement(nodes, node);
}

//=================================================================================================
void Scene::Update(float dt)
{
	LoopAndRemove(particleEmitters, [=](ParticleEmitter* particleEmitter) { return particleEmitter->Update(dt); });
}

//=================================================================================================
void Scene::Clear()
{
	SceneNode::Free(nodes);
	DeleteElements(lights);
	DeleteElements(particleEmitters);
	delete terrain;
}

//=================================================================================================
void Scene::ListNodes(SceneBatch& batch)
{
	FrustumPlanes frustum(batch.camera->matViewProj);

	if(batch.gatherLights)
	{
		activeLights.clear();
		for(Light* light : lights)
		{
			if(frustum.SphereToFrustum(light->pos, light->range))
				activeLights.push_back(light);
		}
	}

	for(SceneNode* node : nodes)
	{
		if(node->visible && frustum.SphereToFrustum(node->pos, node->radius))
		{
			if(batch.gatherLights)
				GatherLights(batch, node);
			batch.Add(node);
		}
	}

	for(ParticleEmitter* particleEmitter : particleEmitters)
	{
		if(frustum.SphereToFrustum(particleEmitter->pos, particleEmitter->radius))
			batch.particleEmitters.push_back(particleEmitter);
	}

	if(terrain)
		terrain->ListVisibleParts(batch.terrainParts, frustum);
}

//=================================================================================================
void Scene::GatherLights(SceneBatch& batch, SceneNode* node)
{
	TopN<Light*, 3, float, std::less<>> best(nullptr, batch.camera->zfar);

	for(Light* light : activeLights)
	{
		float dist = Vec3::Distance(node->pos, light->pos);
		if(dist < light->range + node->radius)
			best.Add(light, dist);
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
