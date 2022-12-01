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
	void Draw(RenderTarget* target);
	void DrawSceneNodes(SceneBatch& batch);
	void DrawAlphaSceneNodes(SceneBatch& batch);

	SuperShader* superShader;
	bool useLighting, useFog, useNormalmap, useSpecularmap;

private:
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	SceneBatch batch;
	Scene* scene;
	Camera* camera;
};
