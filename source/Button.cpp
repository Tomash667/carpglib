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
	State realState = state;
	if(disabled)
		realState = DISABLED;

	if(!custom)
	{
		gui->DrawArea(Box2d::Create(globalPos, size), layout->tex[realState]);

		Rect r = {
			globalPos.x + layout->padding,
			globalPos.y + layout->padding,
			globalPos.x + size.x - layout->padding,
			globalPos.y + size.y - layout->padding
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
			Int2 requiredSize = forceImgSize, imgSize;
			Vec2 scale;
			img->ResizeImage(requiredSize, imgSize, scale);

			// position image
			Vec2 imgPos;
			if(text.empty())
			{
				// when no text put at center
				imgPos = Vec2((float)(size.x - requiredSize.x) / 2 + r.Left(),
					(float)(size.y - requiredSize.y) / 2 + r.Top());
			}
			else
			{
				// put at left
				imgPos = Vec2((float)r.Left(), float(r.Top() + (size.y - requiredSize.y) / 2));
			}

			mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &imgPos);
			gui->DrawSprite2(img, mat, nullptr, &r, Color::White);
			r.Left() += imgSize.x;
		}

		int flags = DTF_CENTER | DTF_VCENTER;
		if(layout->outline)
			flags |= DTF_OUTLINE;
		gui->DrawText(layout->font, text, flags, layout->fontColor[state], r, &r);
	}
	else
		gui->DrawArea(Box2d::Create(globalPos, size), custom->tex[realState]);
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
