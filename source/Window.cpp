#include "Pch.h"
#include "Window.h"

#include "MenuBar.h"

Window::Window(bool fullscreen, bool borderless) : menu(nullptr), fullscreen(fullscreen), borderless(borderless)
{
	size = Int2(300, 200);
}

Window::~Window()
{
}

void Window::Draw()
{
	gui->DrawArea(bodyRect, layout->background);

	if(!borderless)
	{
		gui->DrawArea(headerRect, layout->header);
		if(!text.empty())
		{
			Rect r(headerRect, layout->padding);
			gui->DrawText(layout->font, text, DTF_LEFT | DTF_VCENTER, layout->fontColor, r, &r);
		}
	}

	Box2d* prevClipRect = gui->SetClipRect(&bodyRect);
	Container::Draw();
	gui->SetClipRect(prevClipRect);
}

void Window::Update(float dt)
{
	Container::Update(dt);
}

void Window::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		{
			if(fullscreen)
				size = gui->wndSize;
			if(menu)
				menu->Initialize();
			CalculateArea();
			Int2 offset = Int2(area.v1);
			for(Control* c : ctrls)
			{
				if(c == menu)
					continue;
				if(c->IsDocked())
				{
					c->pos = Int2(0, 0);
					c->size = Int2(area.Size());
				}
				c->globalPos = c->pos + offset + globalPos;
				c->Initialize();
			}
		}
		break;
	case GuiEvent_WindowResize:
		{
			if(fullscreen)
				size = gui->wndSize;
			CalculateArea();
			if(menu)
				menu->Event(GuiEvent_Resize);
			Int2 offset = Int2(area.v1);
			for(Control* c : ctrls)
			{
				if(c == menu)
					continue;
				if(c->IsDocked())
				{
					c->pos = Int2(0, 0);
					c->size = Int2(area.Size());
				}
				c->globalPos = c->pos + offset + globalPos;
				c->Initialize();
			}
		}
		break;
	case GuiEvent_Moved:
		{
			CalculateArea();
			Int2 offset = Int2(area.v1);
			for(Control* c : ctrls)
			{
				c->globalPos = c->pos + globalPos + offset;
				c->Event(GuiEvent_Moved);
			}
		}
		break;
	case GuiEvent_Resize:
		CalculateArea();
		break;
	default:
		if(e >= GuiEvent_Custom)
		{
			if(eventProxy)
				eventProxy->Event(e);
		}
		else
			Container::Event(e);
		break;
	}
}

void Window::SetAreaSize(const Int2& area_size)
{
	Int2 new_size = area_size + Int2(0, layout->headerHeight);
	SetSize(new_size);
}

void Window::SetMenu(MenuBar* _menu)
{
	assert(_menu && !menu && !initialized);
	menu = _menu;
	Add(menu);
}

void Window::CalculateArea()
{
	bodyRect = Box2d(float(globalPos.x), float(globalPos.y), float(globalPos.x + size.x), float(globalPos.y + size.y));
	headerRect = Box2d(float(globalPos.x), float(globalPos.y), float(globalPos.x + size.x), float(globalPos.y + layout->headerHeight));
	area.v1 = Vec2(0, 0);
	if(!borderless)
		area.v1.y += layout->headerHeight;
	if(menu)
		area.v1.y += menu->size.y;
	area.v2 = Vec2(size);
}
