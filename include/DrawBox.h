#pragma once

#include "Control.h"

class DrawBox : public Control
{
public:
	DrawBox();

	void Draw() override;
	Texture* GetTexture() const { return tex; }
	bool IsTexture() const { return tex != nullptr; }
	void SetTexture(Texture* tex);
	void Update(float dt) override;

private:
	Texture* tex;
	Int2 texSize, clickPoint;
	float scale, defaultScale;
	Vec2 move;
	bool clicked;
};
