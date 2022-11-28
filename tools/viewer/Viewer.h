#pragma once

#include "App.h"
#include "Control.h"

class Viewer : public App, public Control
{
	enum CamMode
	{
		Free,
		Left,
		Right,
		Front,
		Back,
		Top,
		Bottom
	};

public:
	Viewer();
	~Viewer();
	bool OnInit() override;
	void OnDraw() override;
	void Draw(ControlDrawData*) override;
	void OnUpdate(float dt) override;
	void SetCamera(CamMode mode);
	void UpdateCamera();

	Scene* scene;
	SceneNode* node;
	Camera* camera;
	string dataDir;
	Font* font;
	float rot, dist;
	MeshInstance* lastMeshInst;
	CamMode mode;
	bool details;
	BasicShader* shader;
};
