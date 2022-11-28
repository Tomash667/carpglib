#include "Pch.h"
#include "Scrollbar.h"

#include "Input.h"

//=================================================================================================
Scrollbar::Scrollbar(bool hscrollbar, bool is_new) : Control(is_new), clicked(false), hscrollbar(hscrollbar), manual_change(false), offset(0.f)
{
}

//=================================================================================================
void Scrollbar::Draw(ControlDrawData* cdd)
{
	gui->DrawArea(Box2d::Create(global_pos, size), layout->tex);

	int s_pos, s_size;
	if(hscrollbar)
	{
		if(part >= total)
		{
			s_pos = 0;
			s_size = size.x;
		}
		else
		{
			s_pos = int(float(offset) / total * size.x);
			s_size = int(float(part) / total * size.x);
		}
		gui->DrawArea(Box2d::Create(Int2(global_pos.x + s_pos, global_pos.y), Int2(s_size, size.y)), layout->tex2);
	}
	else
	{
		if(part >= total)
		{
			s_pos = 0;
			s_size = size.y;
		}
		else
		{
			s_pos = int(float(offset) / total * size.y);
			s_size = int(float(part) / total * size.y);
		}
		gui->DrawArea(Box2d::Create(Int2(global_pos.x, global_pos.y + s_pos), Int2(size.x, s_size)), layout->tex2);
	}
}

//=================================================================================================
void Scrollbar::Update(float dt)
{
	if(!input->Focus())
		return;

	change = 0;

	Int2 cpos = gui->cursor_pos - global_pos;

	if(clicked)
	{
		if(input->Up(Key::LeftButton))
			clicked = false;
		else
		{
			if(hscrollbar)
			{
				int dif = cpos.x - click_pt.x;
				float move = float(dif) * total / size.x;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset + move + float(part) > float(total))
					move = float(max(0, total - part)) - offset;
				else
					changed = false;
				offset += move;
				if(changed)
					click_pt.x += int(move / total * size.x);
				else
					click_pt.x = cpos.x;
			}
			else
			{
				int dif = cpos.y - click_pt.y;
				float move = float(dif) * total / size.y;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset + move + float(part) > float(total))
					move = float(max(0, total - part)) - offset;
				else
					changed = false;
				offset += move;
				if(changed)
					click_pt.y += int(move / total * size.y);
				else
					click_pt.y = cpos.y;
			}
		}
	}
	else if(mouse_focus && input->Pressed(Key::LeftButton))
	{
		if(cpos.x >= 0 && cpos.y >= 0 && cpos.x < size.x && cpos.y < size.y)
		{
			int pos_o = hscrollbar ? int(float(cpos.x) * total / size.x) : int(float(cpos.y) * total / size.y);
			if(hscrollbar ? (pos_o >= offset && pos_o < offset + part) : (pos_o + 2 >= offset && pos_o + 2 < offset + part))
			{
				input->SetState(Key::LeftButton, Input::IS_DOWN);
				clicked = true;
				click_pt = cpos;
			}
			else
			{
				input->SetState(Key::LeftButton, Input::IS_UP);
				if(pos_o < offset)
				{
					if(!manual_change)
					{
						offset -= part;
						if(offset < 0)
							offset = 0;
					}
					else
						change = -1;
				}
				else
				{
					if(!manual_change)
					{
						offset += part;
						if(offset + part > total)
							offset = float(total - part);
					}
					else
						change = 1;
				}
			}
		}

		if(is_new)
			TakeFocus(true);
	}
}

//=================================================================================================
void Scrollbar::LostFocus()
{
	clicked = false;
}

//=================================================================================================
bool Scrollbar::ApplyMouseWheel()
{
	const float wheel = input->GetMouseWheel();
	if(wheel != 0.f)
	{
		LostFocus();
		float mod = (!is_new ? (input->Down(Key::Shift) ? 1.f : 0.2f) : 0.2f);
		float prev_offset = offset;
		offset -= part * wheel * mod;
		if(offset < 0.f)
			offset = 0.f;
		else if(offset + part > total)
			offset = max(0.f, float(total - part));
		return !Equal(offset, prev_offset);
	}
	else
		return false;
}

//=================================================================================================
void Scrollbar::UpdateTotal(int new_total)
{
	total = new_total;
	if(offset + part > total)
		offset = max(0.f, float(total - part));
}

//=================================================================================================
void Scrollbar::UpdateOffset(float _change)
{
	offset += _change;
	if(offset < 0)
		offset = 0;
	else if(offset + part > total)
		offset = max(0.f, float(total - part));
}
