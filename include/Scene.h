#pragma once

//-----------------------------------------------------------------------------
struct Scene
{
	Scene();
	virtual ~Scene() {}
	virtual void Add(SceneNode* node) = 0;
	void Add(Light* light)
	{
		assert(light);
		lights.push_back(light);
	}
	virtual void Remove(SceneNode* node) = 0;
	virtual void Clear() = 0;
	virtual void ListNodes(SceneBatch& batch) = 0;
	void GatherLights(SceneBatch& batch, SceneNode* node);
	Vec4 GetAmbientColor() const;
	Vec4 GetFogColor() const { return fog_color; }
	Vec4 GetFogParams() const;
	Vec4 GetLightColor() const { return light_color; }
	Vec4 GetLightDir() const { return Vec4(light_dir, 1); }

protected:
	void ListNodes(SceneBatch& batch, vector<SceneNode*>& nodes);

	vector<Light*> lights;

public:
	Vec3 light_dir;
	Vec2 fog_range;
	Color clear_color, ambient_color, light_color, fog_color;
	bool use_light_dir;
};
