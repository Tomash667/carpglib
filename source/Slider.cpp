#include "Pch.h"
#include "Slider.h"

//=================================================================================================
Slider::Slider() : width(150), holdVal(0), minstep(false)
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
	bt[0].globalPos = bt[0].pos = globalPos;
	bt[1].globalPos = bt[1].pos = bt[0].pos + Int2(width, 0);

	for(int i = 0; i < 2; ++i)
		bt[i].Draw();

	Rect r0 = { globalPos.x + 32, globalPos.y - 16, globalPos.x + width, globalPos.y + 48 };
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

	if(holdVal > 0)
	{
		if(holdState == -1)
		{
			if(bt[0].state == Button::NONE)
				holdState = 0;
			else
			{
				if(holdTmp > 0.f)
					holdTmp = 0.f;
				holdTmp -= holdVal * dt;
				int count = (int)ceil(holdTmp);
				if(count)
				{
					val += count;
					if(val < minv)
						val = minv;
					holdTmp -= count;
					parent->Event((GuiEvent)id);
					minstep = true;
				}
			}
		}
		else if(holdState == +1)
		{
			if(bt[1].state == Button::NONE)
				holdState = 0;
			else
			{
				if(holdTmp < 0.f)
					holdTmp = 0.f;
				holdTmp += holdVal * dt;
				int count = (int)floor(holdTmp);
				if(count)
				{
					val += count;
					if(val > maxv)
						val = maxv;
					holdTmp -= count;
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
			if(holdVal > 0)
			{
				if(bt[0].state == Button::DOWN)
					holdState = -1;
				else
				{
					holdState = 0;
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
			if(holdVal > 0)
			{
				if(bt[1].state == Button::DOWN)
					holdState = +1;
				else
				{
					holdState = 0;
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
void Slider::SetHold(float holdVal)
{
	this->holdVal = holdVal;
	for(int i = 0; i < 2; ++i)
		bt[i].hold = holdVal > 0;
	holdTmp = 0.f;
	holdState = 0;
}
