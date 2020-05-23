#pragma once

#include "App.h"
#include "Control.h"

class Viewer : public App, public Control
{
public:
	Viewer();
	~Viewer();
	bool OnInit() override;
	void OnDraw() override;
	void Draw(ControlDrawData*) override;
	void OnUpdate(float dt) override;

	Scene* scene;
	SceneNode* node;
	Camera* camera;
	string dataDir;
	Font* font;
	float rot;
	MeshInstance* lastMeshInst;
};
