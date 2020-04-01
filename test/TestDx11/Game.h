#pragma once

#include <App.h>

class Game : public App
{
public:
	~Game();
	bool OnInit() override;
	void OnUpdate(float dt) override;
	void OnDraw() override;

private:
	Scene* scene;
	SceneNode* node;
	Camera* camera;
	float rot;
};
