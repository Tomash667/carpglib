#pragma once

//-----------------------------------------------------------------------------
#include "SceneNode.h"

//-----------------------------------------------------------------------------
class SceneManager
{
public:
	SceneManager();
	void Init();
	void SetScene(Scene* scene, Camera* camera);
	void Draw();
	void Draw(RenderTarget* target);
	void DrawSceneNodes(SceneBatch& batch);
	void DrawAlphaSceneNodes(SceneBatch& batch);

	SuperShader* super_shader;
	bool use_lighting, use_fog, use_normalmap, use_specularmap;

private:
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	SceneBatch batch;
	Scene* scene;
	Camera* camera;
};
