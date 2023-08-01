#pragma once

//-----------------------------------------------------------------------------
#include "Light.h"

//-----------------------------------------------------------------------------
struct Scene
{
	Scene();
	~Scene();
	void Add(SceneNode* node)
	{
		assert(node);
		nodes.push_back(node);
	}
	void Remove(SceneNode* node);
	void Detach(SceneNode* node);
	void Clear();
	void ListNodes(SceneBatch& batch);
	void GatherLights(SceneBatch& batch, SceneNode* node);
	Vec4 GetAmbientColor() const;
	Vec4 GetFogColor() const { return fogColor; }
	Vec4 GetFogParams() const;
	Vec4 GetLightColor() const { return lightColor; }
	Vec4 GetLightDir() const { return Vec4(lightDir, 1); }

	vector<SceneNode*> nodes;
	vector<Light*> lights, activeLights;
	Vec3 lightDir;
	Vec2 fogRange;
	Color clearColor, ambientColor, lightColor, fogColor;
	bool useLightDir;
};
