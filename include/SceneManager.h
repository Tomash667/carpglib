#pragma once

//-----------------------------------------------------------------------------
#include "SceneNode.h"

//-----------------------------------------------------------------------------
class SceneManager
{
public:
	SceneManager();
	void Init();
	void Draw(Scene* scene, Camera* camera, RenderTarget* target);
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	IDirect3DDevice9* device;
	SuperShader* super_shader;
	SceneBatch batch;
	Scene* scene;
	Camera* camera;
	bool use_lighting, use_fog, use_normalmap, use_specularmap;
};
