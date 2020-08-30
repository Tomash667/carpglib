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
	void Add(Light* light)
	{
		assert(light);
		lights.push_back(light);
	}
	void Remove(SceneNode* node)
	{
		assert(node);
		RemoveElement(nodes, node);
	}
	void Clear();
	void ListNodes(SceneBatch& batch);
	void GatherLights(SceneBatch& batch, SceneNode* node);
	Vec4 GetAmbientColor() const;
	Vec4 GetFogColor() const { return fog_color; }
	Vec4 GetFogParams() const;
	Vec4 GetLightColor() const { return light_color; }
	Vec4 GetLightDir() const { return Vec4(light_dir, 1); }

private:
	vector<SceneNode*> nodes;
	vector<Light*> lights;
public:
	Vec3 light_dir;
	Vec2 fog_range;
	Color clear_color, ambient_color, light_color, fog_color;
	bool use_light_dir;
};
