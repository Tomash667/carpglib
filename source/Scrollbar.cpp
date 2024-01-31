#include "Pch.h"
#include "Scrollbar.h"

#include "Input.h"

//=================================================================================================
Scrollbar::Scrollbar(bool hscrollbar, bool isNew) : Control(isNew), clicked(false), hscrollbar(hscrollbar), manualChange(false), offset(0.f)
{
}

//=================================================================================================
void Scrollbar::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size), layout->tex);

	int sPos, sSize;
	if(hscrollbar)
	{
		if(part >= total)
		{
			sPos = 0;
			sSize = size.x;
		}
		else
		{
			sPos = int(float(offset) / total * size.x);
			sSize = int(float(part) / total * size.x);
		}
		gui->DrawArea(Box2d::Create(Int2(globalPos.x + sPos, globalPos.y), Int2(sSize, size.y)), layout->tex2);
	}
	else
	{
		if(part >= total)
		{
			sPos = 0;
			sSize = size.y;
		}
		else
		{
			sPos = int(float(offset) / total * size.y);
			sSize = int(float(part) / total * size.y);
		}
		gui->DrawArea(Box2d::Create(Int2(globalPos.x, globalPos.y + sPos), Int2(size.x, sSize)), layout->tex2);
	}
}

//=================================================================================================
void Scrollbar::Update(float dt)
{
	change = 0;

	Int2 cpos = gui->cursorPos - globalPos;

	if(clicked)
	{
		if(input->Up(Key::LeftButton))
			clicked = false;
		else
		{
			if(hscrollbar)
			{
				int dif = cpos.x - clickedPt.x;
				float move = float(dif)*total / size.x;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset + move + float(part) > float(total))
					move = float(max(0, total - part)) - offset;
				else
					changed = false;
				offset += move;
				if(changed)
					clickedPt.x += int(move / total * size.x);
				else
					clickedPt.x = cpos.x;
			}
			else
			{
				int dif = cpos.y - clickedPt.y;
				float move = float(dif)*total / size.y;
				bool changed = true;
				if(offset + move < 0)
					move = -offset;
				else if(offset + move + float(part) > float(total))
					move = float(max(0, total - part)) - offset;
				else
					changed = false;
				offset += move;
				if(changed)
					clickedPt.y += int(move / total * size.y);
				else
					clickedPt.y = cpos.y;
			}
		}
	}
	else if(mouseFocus && input->Pressed(Key::LeftButton))
	{
		if(cpos.x >= 0 && cpos.y >= 0 && cpos.x < size.x && cpos.y < size.y)
		{
			int posO = hscrollbar ? int(float(cpos.x) * total / size.x) : int(float(cpos.y) * total / size.y);
			if(hscrollbar ? (posO >= offset && posO < offset + part) : (posO + 2 >= offset && posO + 2 < offset + part))
			{
				input->SetState(Key::LeftButton, Input::IS_DOWN);
				clicked = true;
				clickedPt = cpos;
			}
			else
			{
				input->SetState(Key::LeftButton, Input::IS_UP);
				if(posO < offset)
				{
					if(!manualChange)
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
					if(!manualChange)
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

		if(isNew)
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
		float mod = (!isNew ? (input->Down(Key::Shift) ? 1.f : 0.2f) : 0.2f);
		float prevOffset = offset;
		offset -= part * wheel * mod;
		if(offset < 0.f)
			offset = 0.f;
		else if(offset + part > total)
			offset = max(0.f, float(total - part));
		return !Equal(offset, prevOffset);
	}
	else
		return false;
}

//=================================================================================================
void Scrollbar::UpdateTotal(int total)
{
	this->total = total;
	if(offset + part > total)
		offset = max(0.f, float(total - part));
}

//=================================================================================================
void Scrollbar::UpdateOffset(float change)
{
	offset += change;
	if(offset < 0)
		offset = 0;
	else if(offset + part > total)
		offset = max(0.f, float(total - part));
}
