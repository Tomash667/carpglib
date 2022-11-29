#include "Pch.h"
#include "Button.h"

#include "Input.h"

//=================================================================================================
Button::Button() : state(NONE), img(nullptr), hold(false), forceImgSize(0, 0), custom(nullptr)
{
}

//=================================================================================================
void Button::Draw()
{
	State real_state = state;
	if(disabled)
		real_state = DISABLED;

	if(!custom)
	{
		gui->DrawArea(Box2d::Create(globalPos, size), layout->tex[real_state]);

		Rect r = {
			globalPos.x + layout->padding,
			globalPos.y + layout->padding,
			globalPos.x + size.x - layout->padding * 2,
			globalPos.y + size.y - layout->padding * 2
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
			Int2 required_size = forceImgSize, img_size;
			Vec2 scale;
			img->ResizeImage(required_size, img_size, scale);

			// position image
			Vec2 img_pos;
			if(text.empty())
			{
				// when no text put at center
				img_pos = Vec2((float)(size.x - required_size.x) / 2 + r.Left(),
					(float)(size.y - required_size.y) / 2 + r.Top());
			}
			else
			{
				// put at left
				img_pos = Vec2((float)r.Left(), float(r.Top() + (size.y - required_size.y) / 2));
			}

			mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &img_pos);
			gui->DrawSprite2(img, mat, nullptr, &r, Color::White);
			r.Left() += img_size.x;
		}

		gui->DrawText(layout->font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r, &r);
	}
	else
		gui->DrawArea(Box2d::Create(globalPos, size), custom->tex[real_state]);
}

//=================================================================================================
void Button::Update(float dt)
{
	if(state == DISABLED || disabled)
		return;

	if(input->Focus() && mouseFocus && IsInside(gui->cursorPos))
	{
		gui->SetCursorMode(CURSOR_HOVER);
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
