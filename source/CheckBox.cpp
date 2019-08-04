#include "EnginePch.h"
#include "EngineCore.h"
#include "CheckBox.h"
#include "Input.h"

//=================================================================================================
CheckBox::CheckBox(Cstring text, bool checked) : text(text), checked(checked), state(NONE), bt_size(32, 32), radiobox(false), id(-1)
{
}

//=================================================================================================
void CheckBox::Draw(ControlDrawData*)
{
	gui->DrawArea(Box2d::Create(global_pos, bt_size), layout->tex[state]);

	if(checked)
		gui->DrawArea(Box2d::Create(global_pos, layout->tick.size), layout->tick);

	Rect r = { global_pos.x + bt_size.x + 4, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(layout->font, text, DTF_VCENTER, Color::Black, r);
}

//=================================================================================================
void CheckBox::Update(float dt)
{
	if(state == DISABLED)
		return;

	if(input->Focus() && mouse_focus && PointInRect(gui->cursor_pos, global_pos, bt_size))
	{
		gui->cursor_mode = CURSOR_HOVER;
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
