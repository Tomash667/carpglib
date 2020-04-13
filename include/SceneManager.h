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
	void DrawSceneNodes(SceneBatch& batch);
	void DrawAlphaSceneNodes(SceneBatch& batch);

	SuperShader* super_shader;
	Scene* scene;
	Camera* camera;
	bool use_lighting, use_fog, use_normalmap, use_specularmap;

private:
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	SceneBatch batch;
};
