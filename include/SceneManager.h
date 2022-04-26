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
	void Prepare();
	void Draw();
	void Draw(RenderTarget* target);
	void DrawSceneNodes();
	void DrawSceneNodes(SceneBatch& batch);
	void DrawAlphaSceneNodes(SceneBatch& batch);
	void DrawSkybox(Mesh* mesh);

	Scene* GetScene() { return scene; }

	bool useLighting, useFog, useNormalmap, useSpecularmap;

private:
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<SceneNodeGroup>& groups);
	void DrawAlphaSceneNodes(const vector<SceneNode*>& nodes);

	SceneBatch batch;
	Scene* scene;
	Camera* camera;
	SuperShader* superShader;
	SkyboxShader* skyboxShader;
};
