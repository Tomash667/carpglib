#include "EnginePch.h"
#include "EngineCore.h"
#include "DrawBox.h"
#include "Input.h"

DrawBox::DrawBox() : tex(nullptr), clicked(false), default_scale(1.f)
{
}

void DrawBox::Draw(ControlDrawData*)
{
	Rect r = Rect::Create(global_pos, size);
	gui->DrawArea(Color(150, 150, 150), r);

	if(tex)
	{
		Matrix m;
		Vec2 scaled_tex_size = Vec2(tex_size) * scale;
		Vec2 max_pos = scaled_tex_size - Vec2(size);
		Vec2 p = Vec2(max_pos.x * -move.x / 100, max_pos.y * -move.y / 100) + Vec2(global_pos);
		m = Matrix::Transform2D(nullptr, 0.f, &Vec2(scale, scale), nullptr, 0.f, &p);
		gui->DrawSprite2(tex, m, nullptr, &r);
	}
}

void DrawBox::Update(float dt)
{
	if(mouse_focus)
	{
		float change = gui->mouse_wheel;
		if(change > 0)
		{
			while(change > 0)
			{
				float prev_scale = scale;
				scale *= 1.1f;
				if(prev_scale < default_scale && scale > default_scale)
					scale = default_scale;
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
				if(prev_scale > default_scale && scale < default_scale)
					scale = default_scale;
				change -= 1.f;
			}
		}

		if(!clicked && input->Down(Key::LeftButton))
		{
			clicked = true;
			click_point = gui->cursor_pos;
		}
	}

	if(clicked)
	{
		if(input->Up(Key::LeftButton))
			clicked = false;
		else
		{
			Int2 dif = click_point - gui->cursor_pos;
			gui->cursor_pos = click_point;
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

	tex_size = t->GetSize();
	Vec2 sizef = Vec2(size);
	Vec2 scale2 = Vec2(sizef.x / tex_size.x, sizef.y / tex_size.y);
	scale = min(scale2.x, scale2.y);
	default_scale = scale;
	move = Vec2(0, 0);
}
