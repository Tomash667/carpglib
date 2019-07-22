#include "EnginePch.h"
#include "EngineCore.h"
#include "CheckBox.h"
#include "Button.h"
#include "Input.h"

//-----------------------------------------------------------------------------
TEX CheckBox::tTick;

//=================================================================================================
CheckBox::CheckBox(StringOrCstring text, bool checked) : text(text.c_str()), checked(checked), state(NONE), bt_size(32, 32), radiobox(false), id(-1)
{
}

//=================================================================================================
void CheckBox::Draw(ControlDrawData* cdd/* =nullptr */)
{
	gui->DrawItem(Button::tex[state], global_pos, bt_size, Color::White, 12);

	if(checked)
		gui->DrawSprite(tTick, global_pos);

	Rect r = { global_pos.x + bt_size.x + 4, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(gui->default_font, text, DTF_VCENTER, Color::Black, r);
}

//=================================================================================================
void CheckBox::Update(float dt)
{
	if(state == DISABLED)
		return;

	if(input->Focus() && mouse_focus && PointInRect(gui->cursor_pos, global_pos, bt_size))
	{
		gui->cursor_mode = CURSOR_HAND;
		if(state == PRESSED)
		{
			if(input->Up(Key::LeftButton))
			{
				state = FLASH;
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
			state = PRESSED;
		else
			state = FLASH;
	}
	else
		state = NONE;
}
