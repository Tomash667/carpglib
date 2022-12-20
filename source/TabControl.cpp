#include "Pch.h"
#include "TabControl.h"

#include "Input.h"
#include "Panel.h"

static ObjectPool<TabControl::Tab> tabPool;

TabControl::TabControl(bool ownPanels) : selected(nullptr), hover(nullptr), ownPanels(ownPanels), tabOffset(0), tabOffsetMax(0), arrowHover(0)
{
}

TabControl::~TabControl()
{
	Clear();
}

void TabControl::Dock(Control* c)
{
	assert(c);
	Int2 areaPos = GetAreaPos() + globalPos;
	Int2 areaSize = GetAreaSize();
	if(c->globalPos != areaPos)
	{
		c->globalPos = areaPos;
		if(c->IsInitialized())
			c->Event(GuiEvent_Moved);
	}
	if(c->size != areaSize)
	{
		c->size = areaSize;
		if(c->IsInitialized())
			c->Event(GuiEvent_Resize);
	}
}

void TabControl::Draw()
{
	Box2d bodyRect = Box2d::Create(globalPos, size);
	gui->DrawArea(bodyRect, layout->background);

	gui->DrawArea(line, layout->line);

	Box2d rectf;
	if(tabOffset > 0)
	{
		rectf.v1.x = (float)globalPos.x;
		rectf.v1.y = (float)globalPos.y + (height - layout->buttonPrev.size.y) / 2;
		rectf.v2 = rectf.v1 + Vec2(layout->buttonPrev.size);
		gui->DrawArea(rectf, arrowHover == -1 ? layout->buttonPrevHover : layout->buttonPrev);
	}

	if(tabOffsetMax != tabs.size())
	{
		rectf.v1.x = (float)globalPos.x + size.x - layout->buttonNext.size.x;
		rectf.v1.y = (float)globalPos.y + (height - layout->buttonNext.size.y) / 2;
		rectf.v2 = rectf.v1 + Vec2(layout->buttonNext.size);
		gui->DrawArea(rectf, arrowHover == 1 ? layout->buttonNextHover : layout->buttonNext);
	}

	Rect rect;
	for(int i = tabOffset; i < tabOffsetMax; ++i)
	{
		Tab* tab = tabs[i];
		AreaLayout* button;
		AreaLayout* close;
		Color color;
		switch(tab->mode)
		{
		default:
		case Tab::Up:
			button = &layout->button;
			color = layout->fontColor;
			break;
		case Tab::Hover:
			button = &layout->buttonHover;
			color = layout->fontColorHover;
			break;
		case Tab::Down:
			button = &layout->buttonDown;
			color = layout->fontColorDown;
			break;
		}
		if(tab->closeHover)
			close = &layout->closeHover;
		else
			close = &layout->close;

		gui->DrawArea(tab->rect, *button);
		rect = Rect(tab->rect, layout->padding);
		gui->DrawText(layout->font, tab->text, DTF_LEFT | DTF_VCENTER, color, rect);
		gui->DrawArea(tab->closeRect, *close);

		if(tab->haveChanges)
			gui->DrawArea(Color::Red, Int2(tab->rect.LeftTop()), Int2(2, (int)tab->rect.SizeY()));
	}

	if(selected)
		selected->panel->Draw();
}

void TabControl::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		Update(true, true);
		for(Tab* tab : tabs)
			tab->panel->Initialize();
		break;
	case GuiEvent_Moved:
		Update(true, false);
		break;
	case GuiEvent_Resize:
		Update(false, true);
		break;
	}
}

void TabControl::Update(float dt)
{
	arrowHover = 0;

	if(hover)
	{
		hover->mode = Tab::Up;
		hover->closeHover = false;
		hover = nullptr;
	}
	if(selected)
		selected->closeHover = false;

	if(mouseFocus && IsInside(gui->cursorPos))
	{
		Box2d rectf;
		if(tabOffset > 0)
		{
			rectf.v1.x = (float)globalPos.x;
			rectf.v1.y = (float)globalPos.y + (height - layout->buttonPrev.size.y) / 2;
			rectf.v2 = rectf.v1 + Vec2(layout->buttonPrev.size);
			if(rectf.IsInside(gui->cursorPos))
			{
				arrowHover = -1;
				if(input->Pressed(Key::LeftButton))
				{
					--tabOffset;
					CalculateTabOffsetMax();
				}
			}
		}

		if(tabOffsetMax != tabs.size())
		{
			rectf.v1.x = (float)globalPos.x + size.x - layout->buttonNext.size.x;
			rectf.v1.y = (float)globalPos.y + (height - layout->buttonNext.size.y) / 2;
			rectf.v2 = rectf.v1 + Vec2(layout->buttonNext.size);
			if(rectf.IsInside(gui->cursorPos))
			{
				arrowHover = 1;
				if(input->Pressed(Key::LeftButton))
				{
					++tabOffset;
					CalculateTabOffsetMax();
				}
			}
		}

		for(int i = tabOffset; i < tabOffsetMax; ++i)
		{
			Tab* tab = tabs[i];
			if(tab->rect.IsInside(gui->cursorPos))
			{
				if(tab != selected)
				{
					hover = tab;
					hover->mode = Tab::Hover;
				}
				if(tab->closeRect.IsInside(gui->cursorPos))
				{
					tab->closeHover = true;
					if(input->Pressed(Key::LeftButton))
						tab->Close();
				}
				else if(input->Pressed(Key::LeftButton) && tab != selected)
				{
					if(SelectInternal(tab))
						hover = nullptr;
				}
				else if(input->Pressed(Key::MiddleButton))
					tab->Close();
				break;
			}
		}
	}

	if(selected)
		UpdateControl(selected->panel, dt);
}

TabControl::Tab* TabControl::AddTab(cstring id, cstring text, Panel* panel, bool select)
{
	assert(id && text && panel);

	Tab* tab = new Tab;
	tab->parent = this;
	tab->id = id;
	tab->text = text;
	tab->panel = panel;
	tab->mode = Tab::Up;
	tab->closeHover = false;
	tab->size = layout->font->CalculateSize(text) + layout->padding * 2 + Int2(layout->close.size.x + layout->padding.x, 0);
	tab->haveChanges = false;
	tabs.push_back(tab);
	CalculateTabOffsetMax();

	if(IsInitialized())
		panel->Initialize();
	if(panel->IsDocked())
		Dock(panel);

	bool selected = false;
	if(select || tabs.size() == 1u)
	{
		if(SelectInternal(tab))
		{
			ScrollTo(tab);
			selected = true;
		}
	}
	if(selected)
		ScrollTo(tab);

	return tab;
}

void TabControl::Clear()
{
	if(ownPanels)
	{
		for(Tab* tab : tabs)
			delete tab->panel;
	}
	tabPool.Free(tabs);
	selected = nullptr;
	hover = nullptr;
	totalWidth = 0;
	arrowHover = 0;
}

TabControl::Tab* TabControl::Find(cstring id)
{
	for(Tab* tab : tabs)
	{
		if(tab->id == id)
			return tab;
	}
	return nullptr;
}

Int2 TabControl::GetAreaPos() const
{
	int height = layout->font->height + layout->paddingActive.y;
	return Int2(0, height);
}

Int2 TabControl::GetAreaSize() const
{
	int height = layout->font->height + layout->paddingActive.y;
	return Int2(size.x, size.y - height);
}

void TabControl::Close(Tab* tab)
{
	assert(tab);
	if(tab == hover)
		hover = nullptr;
	if(!handler(A_BEFORE_CLOSE, (int)tab))
		return;
	int index = GetIndex(tabs, tab);
	if(tab == selected)
	{
		// select next tab or previous if not exists
		Tab* newSelected;
		if(index == tabs.size() - 1)
		{
			if(index == 0)
				newSelected = nullptr;
			else
				newSelected = tabs[index - 1];
		}
		else
			newSelected = tabs[index + 1];
		SelectInternal(newSelected);
		if(newSelected == hover)
			hover = nullptr;
	}
	tabPool.Free(tab);
	tabs.erase(tabs.begin() + index);
	if(tabOffset > (int)tabs.size())
		--tabOffset;
	CalculateTabOffsetMax();
}

void TabControl::Select(Tab* tab, bool scrollTo)
{
	if(tab == selected)
		return;
	if(SelectInternal(tab))
	{
		if(selected == hover)
			hover = nullptr;
		if(tab && scrollTo)
			ScrollTo(tab);
	}
}

void TabControl::Update(bool move, bool resize)
{
	if(move)
	{
		if(!IsDocked())
			globalPos = parent->globalPos + pos;
		CalculateRect();
	}

	if(resize)
	{
		height = layout->font->height + layout->paddingActive.y;
		allowedSize = (int)(size.x - layout->buttonPrev.size.x - layout->buttonNext.region.SizeX());
	}

	int p = (layout->paddingActive.y - layout->padding.y);
	line.v1.x = (float)globalPos.x;
	line.v2.x = (float)globalPos.x + size.x;
	line.v1.y = (float)(globalPos.y + height - p / 2);
	line.v2.y = line.v1.y + p / 2;
}

void TabControl::CalculateRect()
{
	totalWidth = (int)layout->buttonPrev.size.x;
	for(int i = tabOffset; i < tabOffsetMax; ++i)
	{
		Tab* tab = tabs[i];
		CalculateRect(*tab, totalWidth);
		totalWidth += tab->size.x;
	}
}

void TabControl::CalculateRect(Tab& tab, int offset)
{
	Int2 pad = (tab.mode == Tab::Down ? layout->paddingActive : layout->padding);
	int p = (layout->paddingActive.y - layout->padding.y);

	tab.rect.v1 = Vec2((float)globalPos.x + offset, (float)globalPos.y);
	tab.rect.v2 = tab.rect.v1 + Vec2(tab.size);
	if(tab.mode == Tab::Down)
		tab.rect.v2.y += (layout->paddingActive.y - layout->padding.y);
	else
	{
		tab.rect.v1.y += p / 2;
		tab.rect.v1.y += p;
	}

	tab.closeRect.v1.x = tab.rect.v2.x - pad.x - layout->close.size.x;
	tab.closeRect.v1.y = tab.rect.v1.y + (tab.rect.SizeY() - layout->close.size.y) / 2;
	tab.closeRect.v2 = tab.closeRect.v1 + Vec2(layout->close.size);
}

bool TabControl::SelectInternal(Tab* tab)
{
	if(tab == selected)
		return true;

	if(!handler(A_BEFORE_CHANGE, (int)selected))
		return false;

	if(selected)
	{
		selected->mode = Tab::Up;
		CalculateRect(*selected, (int)selected->rect.v1.x);
	}

	selected = tab;
	if(selected)
	{
		selected->mode = Tab::Down;
		if(!selected->panel->IsInitialized())
		{
			selected->panel->pos = Int2(1, height + 1);
			selected->panel->globalPos = globalPos + selected->panel->pos;
			selected->panel->size = size - Int2(2, height + 2);
			selected->panel->Initialize();
		}
		selected->panel->Show();
		CalculateRect(*selected, (int)selected->rect.v1.x);
	}

	handler(A_CHANGED, (int)selected);
	return true;
}

void TabControl::ScrollTo(Tab* tab)
{
	assert(tab);
	int index = GetIndex(tabs, tab);
	if(index >= tabOffset && index < tabOffsetMax)
		return;
	tabOffset = index;
	CalculateTabOffsetMax();
}

void TabControl::CalculateTabOffsetMax()
{
	int width = 0;

	for(int i = tabOffset, count = tabs.size(); i < count; ++i)
	{
		width += tabs[i]->size.x;
		if(width > allowedSize)
		{
			tabOffsetMax = i;
			CalculateRect();
			return;
		}
	}

	tabOffsetMax = tabs.size();

	for(int i = tabOffset - 1; i >= 0; --i)
	{
		width += tabs[i]->size.x;
		if(width < allowedSize)
			--tabOffset;
	}

	CalculateRect();
}
