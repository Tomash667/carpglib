#pragma once

#include "Control.h"

class DrawBox : public Control
{
public:
	DrawBox();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	Texture* GetTexture() const { return tex; }

	bool IsTexture() const { return tex != nullptr; }

	void SetTexture(Texture* tex);

private:
	Texture* tex;
	Int2 tex_size, click_point;
	float scale, default_scale;
	Vec2 move;
	bool clicked;
};
