#pragma once

//-----------------------------------------------------------------------------
#include "SceneNode.h"

//-----------------------------------------------------------------------------
class SceneManager
{
public:
	SceneManager();
	void Init();
	void Draw();
	void Draw(Scene* scene, Camera* camera, RenderTarget* target);
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	IDirect3DDevice9* device;
	SuperShader* super_shader;
	SkyboxShader* skybox_shader;
	SceneBatch batch;
	Scene* active_scene;
	Scene* scene;
	Camera* active_camera;
	Camera* camera;
	bool use_lighting, use_fog, use_normalmap, use_specularmap;
};
