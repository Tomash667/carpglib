#include "Pch.h"
#include "MenuStrip.h"

#include "GuiElement.h"
#include "Input.h"
#include "MenuBar.h"
#include "Overlay.h"

MenuStrip::MenuStrip(vector<SimpleMenuCtor>& _items, int minWIdth) : selected(nullptr)
{
	items.resize(_items.size());
	for(uint i = 0, size = _items.size(); i < size; ++i)
	{
		SimpleMenuCtor& item1 = _items[i];
		Item& item2 = items[i];
		item2.text = item1.text;
		item2.action = item1.id;
		item2.hover = false;
		item2.index = i;
		item2.enabled = true;
	}

	CalculateWidth(minWIdth);
}

MenuStrip::MenuStrip(vector<GuiElement*>& _items, int minWIdth) : selected(nullptr)
{
	items.resize(_items.size());
	for(uint i = 0, size = _items.size(); i < size; ++i)
	{
		GuiElement* e = _items[i];
		Item& item2 = items[i];
		item2.text = e->ToString();
		item2.action = e->value;
		item2.hover = false;
		item2.index = i;
		item2.enabled = true;
	}

	CalculateWidth(minWIdth);
}

void MenuStrip::CalculateWidth(int minWIdth)
{
	int maxWidth = 0;
	Font* font = layout->font;

	for(auto& item : items)
	{
		int width = font->CalculateSize(item.text).x;
		if(width > maxWidth)
			maxWidth = width;
	}

	size = Int2(maxWidth + (layout->padding.x + layout->itemPadding.x) * 2,
		(font->height + (layout->itemPadding.y) * 2) * items.size() + layout->padding.y * 2);

	if(size.x < minWIdth)
		size.x = minWIdth;

	SetOnCharHandler(true);
}

MenuStrip::~MenuStrip()
{
}

void MenuStrip::Draw()
{
	Box2d area = Box2d::Create(globalPos, size);
	gui->DrawArea(area, layout->background);

	Vec2 itemsSize((float)size.x - (layout->padding.x) * 2,
		(float)layout->font->height + layout->itemPadding.y * 2);
	area.v1 = Vec2(globalPos + layout->padding);
	area.v2 = area.v1 + itemsSize;
	float offset = itemsSize.y;
	Rect r;

	for(Item& item : items)
	{
		if(item.hover)
			gui->DrawArea(area, layout->buttonHover);

		Color color;
		if(!item.enabled)
			color = layout->fontColorDisabled;
		else if(item.hover)
			color = layout->fontColorHover;
		else
			color = layout->fontColor;
		r = Rect(area, layout->itemPadding);
		gui->DrawText(layout->font, item.text, DTF_LEFT, color, r);

		area += Vec2(0, offset);
	}
}

void MenuStrip::OnChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = tolower(c);

	bool first = true;
	int startIndex;
	if(selected)
		startIndex = selected->index;
	else
		startIndex = 0;
	int index = startIndex;

	while(true)
	{
		auto& item = items[index];
		char startsWith = tolower(item.text[0]);
		if(startsWith == c)
		{
			if(index == startIndex && selected && first)
				first = false;
			else
			{
				if(selected)
					selected->hover = false;
				selected = &item;
				selected->hover = true;
				return;
			}
		}
		else if(index == startIndex)
		{
			if(first)
				first = false;
			else
				return;
		}
		index = (index + 1) % items.size();
	}
}

void MenuStrip::Update(float dt)
{
	UpdateMouse();
	UpdateKeyboard();
}

void MenuStrip::UpdateMouse()
{
	if(!mouseFocus)
	{
		if(!focus)
		{
			if(selected)
				selected->hover = false;
			selected = nullptr;
		}
		return;
	}

	Box2d area = Box2d::Create(globalPos, size);
	if(!area.IsInside(gui->cursorPos))
	{
		if(gui->MouseMoved())
		{
			if(selected)
			{
				selected->hover = false;
				selected = nullptr;
			}
		}
		return;
	}

	Vec2 itemsSize((float)size.x - (layout->padding.x) * 2,
		(float)layout->font->height + layout->itemPadding.y * 2);
	area.v1 = Vec2(globalPos + layout->padding);
	area.v2 = area.v1 + itemsSize;
	float offset = itemsSize.y;

	for(Item& item : items)
	{
		if(area.IsInside(gui->cursorPos))
		{
			if(item.enabled && (gui->MouseMoved() || input->Pressed(Key::LeftButton)))
			{
				if(selected)
					selected->hover = false;
				selected = &item;
				selected->hover = true;
				if(input->PressedRelease(Key::LeftButton))
				{
					if(handler)
						handler(item.action);
					gui->GetOverlay()->CloseMenu(this);
				}
			}
			break;
		}

		area += Vec2(0, offset);
	}
}

void MenuStrip::UpdateKeyboard()
{
	if(!focus)
		return;

	if(input->PressedRelease(Key::Down))
		ChangeIndex(+1);
	else if(input->PressedRelease(Key::Up))
		ChangeIndex(-1);
	else if(input->PressedRelease(Key::Left))
	{
		if(parentMenuBar)
			parentMenuBar->ChangeMenu(-1);
	}
	else if(input->PressedRelease(Key::Right))
	{
		if(parentMenuBar)
			parentMenuBar->ChangeMenu(+1);
	}
	else if(input->PressedRelease(Key::Enter))
	{
		if(selected)
		{
			if(handler)
				handler(selected->action);
			gui->GetOverlay()->CloseMenu(this);
		}
	}
	else if(input->PressedRelease(Key::Escape))
		gui->GetOverlay()->CloseMenu(this);
}

void MenuStrip::ShowAt(const Int2& _pos)
{
	if(selected)
		selected->hover = false;
	selected = nullptr;
	pos = _pos;
	globalPos = pos;
	Show();
}

void MenuStrip::ShowMenu(const Int2& _pos)
{
	gui->GetOverlay()->ShowMenu(this, _pos);
}

void MenuStrip::SetSelectedIndex(int index)
{
	assert(index >= 0 && index < (int)items.size());
	if(!items[index].enabled)
		return;
	if(selected)
		selected->hover = false;
	selected = &items[index];
	selected->hover = true;
}

MenuStrip::Item* MenuStrip::FindItem(int action)
{
	for(Item& item : items)
	{
		if(item.action == action)
			return &item;
	}
	return nullptr;
}

void MenuStrip::ChangeIndex(int dif)
{
	if(items.empty())
		return;

	int start;
	if(!selected)
	{
		selected = &items[0];
		start = 0;
	}
	else
	{
		selected->hover = false;
		selected = &items[Modulo(selected->index + dif, items.size())];
		start = selected->index;
	}

	while(!selected->enabled)
	{
		selected = &items[Modulo(selected->index + dif, items.size())];
		if(selected->index == start)
		{
			// all items disabled
			selected = nullptr;
			return;
		}
	}

	selected->hover = true;
}

bool MenuStrip::IsOpen()
{
	return gui->GetOverlay()->IsOpen(this);
}
