#include "EnginePch.h"
#include "EngineCore.h"
#include "Button.h"
#include "Input.h"

//=================================================================================================
Button::Button() : state(NONE), img(nullptr), hold(false), force_img_size(0, 0), custom(nullptr)
{
}

//=================================================================================================
void Button::Draw(ControlDrawData*)
{
	State real_state = state;
	if(disabled)
		real_state = DISABLED;

	if(!custom)
	{
		gui->DrawArea(Box2d::Create(global_pos, size), layout->tex[real_state]);

		Rect r = {
			global_pos.x + 4,
			global_pos.y + 4,
			global_pos.x + size.x - 8,
			global_pos.y + size.y - 8
		};

		if(state == DOWN)
			r += Int2(1, 1);

		if(img)
		{
			Int2 pt = r.LeftTop();
			if(state == DOWN)
			{
				++pt.x;
				++pt.y;
			}

			Matrix mat;
			Int2 required_size = force_img_size, img_size;
			Vec2 scale;
			img->ResizeImage(required_size, img_size, scale);
			mat = Matrix::Transform2D(&Vec2(float(img_size.x) / 2, float(img_size.y) / 2), 0.f, &scale, nullptr, 0.f,
				&Vec2((float)r.Left(), float(r.Top() + (required_size.y - img_size.y) / 2)));
			gui->DrawSprite2(img, mat, nullptr, &r, Color::White);
			r.Left() += img_size.x;
		}

		gui->DrawText(layout->font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r, &r);
	}
	else
		gui->DrawArea(Box2d::Create(global_pos, size), custom->tex[real_state]);
}

//=================================================================================================
void Button::Update(float dt)
{
	if(state == DISABLED || disabled)
		return;

	if(input->Focus() && mouse_focus && IsInside(gui->cursor_pos))
	{
		gui->cursor_mode = CURSOR_HOVER;
		if(state == DOWN)
		{
			bool apply = false;
			if(input->Up(Key::LeftButton))
			{
				state = HOVER;
				apply = true;
			}
			else if(hold)
				apply = true;
			if(apply)
			{
				if(handler)
					handler(id);
				else
					parent->Event((GuiEvent)id);
			}
		}
		else if(input->Pressed(Key::LeftButton))
			state = DOWN;
		else
			state = HOVER;
	}
	else
		state = NONE;
}
