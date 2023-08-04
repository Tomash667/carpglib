#include "Pch.h"
#include "CheckBox.h"

#include "Input.h"

//=================================================================================================
CheckBox::CheckBox(Cstring text, bool checked) : text(text), checked(checked), state(NONE), btSize(32, 32), radiobox(false), id(-1)
{
}

//=================================================================================================
void CheckBox::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, btSize), layout->tex[state]);

	if(checked)
		gui->DrawArea(Box2d::Create(globalPos, layout->tick.size), layout->tick);

	Rect r = { globalPos.x + btSize.x + 4, globalPos.y, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(layout->font, text, DTF_VCENTER, Color::Black, r);
}

//=================================================================================================
void CheckBox::Update(float dt)
{
	if(state == DISABLED)
		return;

	if(input->Focus() && mouseFocus && Rect::IsInside(gui->cursorPos, globalPos, btSize))
	{
		gui->SetCursorMode(CURSOR_HOVER);
		if(state == DOWN)
		{
			if(input->Up(Key::LeftButton))
			{
				state = HOVER;
				if(radiobox)
				{
					if(!checked)
					{
						checked = true;
						parent->Event((GuiEvent)id);
					}
				}
				else
				{
					checked = !checked;
					parent->Event((GuiEvent)id);
				}
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
