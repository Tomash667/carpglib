#include "Pch.h"
#include "DrawBox.h"

#include "Input.h"

DrawBox::DrawBox() : Control(true), tex(nullptr), clicked(false), defaultScale(1.f)
{
}

void DrawBox::Draw()
{
	Rect r = Rect::Create(globalPos, size);
	gui->DrawArea(Color(150, 150, 150), r);

	if(tex)
	{
		const Vec2 scaled_tex_size = Vec2(texSize) * scale;
		const Vec2 max_pos = scaled_tex_size - Vec2(size);
		const Vec2 p = Vec2(max_pos.x * -move.x / 100, max_pos.y * -move.y / 100) + Vec2(globalPos);
		const Vec2 scaleV = Vec2(scale, scale);
		const Matrix m = Matrix::Transform2D(nullptr, 0.f, &scaleV, nullptr, 0.f, &p);
		gui->DrawSprite2(tex, m, nullptr, &r);
	}
}

void DrawBox::Update(float dt)
{
	if(mouseFocus)
	{
		float change = input->GetMouseWheel();
		if(change > 0)
		{
			while(change > 0)
			{
				float prev_scale = scale;
				scale *= 1.1f;
				if(prev_scale < defaultScale && scale > defaultScale)
					scale = defaultScale;
				change -= 1.f;
			}
		}
		else if(change < 0)
		{
			change = -change;
			while(change > 0)
			{
				float prev_scale = scale;
				scale *= 0.9f;
				if(prev_scale > defaultScale && scale < defaultScale)
					scale = defaultScale;
				change -= 1.f;
			}
		}

		if(!clicked && input->Down(Key::LeftButton))
		{
			clicked = true;
			clickPoint = gui->cursorPos;
		}
	}

	if(clicked)
	{
		if(input->Up(Key::LeftButton))
			clicked = false;
		else
		{
			Int2 dif = clickPoint - gui->cursorPos;
			gui->cursorPos = clickPoint;
			move -= Vec2(dif) / 2;
			move.x = Clamp(move.x, 0.f, 100.f);
			move.y = Clamp(move.y, 0.f, 100.f);
		}
	}
}

void DrawBox::SetTexture(Texture* t)
{
	assert(t && t->IsLoaded());
	tex = t;

	texSize = t->GetSize();
	Vec2 sizef = Vec2(size);
	Vec2 scale2 = Vec2(sizef.x / texSize.x, sizef.y / texSize.y);
	scale = min(scale2.x, scale2.y);
	defaultScale = scale;
	move = Vec2(0, 0);
}
