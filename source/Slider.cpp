#include "Pch.h"
#include "Slider.h"

//=================================================================================================
Slider::Slider() : hold(false), minstep(false)
{
	bt[0].text = '<';
	bt[0].id = GuiEvent_Custom;
	bt[0].parent = this;
	bt[0].size = Int2(32, 32);

	bt[1].text = '>';
	bt[1].id = GuiEvent_Custom + 1;
	bt[1].parent = this;
	bt[1].size = Int2(32, 32);
}

//=================================================================================================
void Slider::Draw()
{
	const int D = 150;

	bt[0].globalPos = bt[0].pos = globalPos;
	bt[1].globalPos = bt[1].pos = bt[0].pos + Int2(D, 0);

	for(int i = 0; i < 2; ++i)
		bt[i].Draw();

	Rect r0 = { globalPos.x + 32, globalPos.y - 16, globalPos.x + D, globalPos.y + 48 };
	gui->DrawText(layout->font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r0);
}

//=================================================================================================
void Slider::Update(float dt)
{
	for(int i = 0; i < 2; ++i)
	{
		bt[i].mouseFocus = mouseFocus;
		bt[i].Update(dt);
	}

	if(hold)
	{
		if(hold_state == -1)
		{
			if(bt[0].state == Button::NONE)
				hold_state = 0;
			else
			{
				if(hold_tmp > 0.f)
					hold_tmp = 0.f;
				hold_tmp -= hold_val*dt;
				int count = (int)ceil(hold_tmp);
				if(count)
				{
					val += count;
					if(val < minv)
						val = minv;
					hold_tmp -= count;
					parent->Event((GuiEvent)id);
					minstep = true;
				}
			}
		}
		else if(hold_state == +1)
		{
			if(bt[1].state == Button::NONE)
				hold_state = 0;
			else
			{
				if(hold_tmp < 0.f)
					hold_tmp = 0.f;
				hold_tmp += hold_val*dt;
				int count = (int)floor(hold_tmp);
				if(count)
				{
					val += count;
					if(val > maxv)
						val = maxv;
					hold_tmp -= count;
					parent->Event((GuiEvent)id);
					minstep = true;
				}
			}
		}
	}
}

//=================================================================================================
void Slider::Event(GuiEvent e)
{
	if(e == GuiEvent_Custom)
	{
		if(val != minv)
		{
			if(hold)
			{
				if(bt[0].state == Button::DOWN)
					hold_state = -1;
				else
				{
					hold_state = 0;
					if(!minstep)
					{
						--val;
						parent->Event((GuiEvent)id);
					}
					minstep = false;
				}
			}
			else
			{
				--val;
				parent->Event((GuiEvent)id);
			}
		}
	}
	else if(e == GuiEvent_Custom + 1)
	{
		if(val != maxv)
		{
			if(hold)
			{
				if(bt[1].state == Button::DOWN)
					hold_state = +1;
				else
				{
					hold_state = 0;
					if(!minstep)
					{
						++val;
						parent->Event((GuiEvent)id);
					}
					minstep = false;
				}
			}
			else
			{
				++val;
				parent->Event((GuiEvent)id);
			}
		}
	}
}

//=================================================================================================
void Slider::SetHold(bool _hold)
{
	hold = _hold;
	for(int i = 0; i < 2; ++i)
		bt[i].hold = hold;
	hold_tmp = 0.f;
	hold_state = 0;
}
