#pragma once

#include <App.h>

class Game : public App
{
public:
	void OnUpdate(float dt) override;
	void OnDraw() override;
};
