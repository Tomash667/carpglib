#include "Pch.h"
#include "MenuStrip.h"

#include "GuiElement.h"
#include "Input.h"
#include "MenuBar.h"
#include "Overlay.h"

MenuStrip::MenuStrip(vector<SimpleMenuCtor>& _items, int min_width) : selected(nullptr)
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

	CalculateWidth(min_width);
}

MenuStrip::MenuStrip(vector<GuiElement*>& _items, int min_width) : selected(nullptr)
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

	CalculateWidth(min_width);
}

void MenuStrip::CalculateWidth(int min_width)
{
	int max_width = 0;
	Font* font = layout->font;

	for(auto& item : items)
	{
		int width = font->CalculateSize(item.text).x;
		if(width > max_width)
			max_width = width;
	}

	size = Int2(max_width + (layout->padding.x + layout->item_padding.x) * 2,
		(font->height + (layout->item_padding.y) * 2) * items.size() + layout->padding.y * 2);

	if(size.x < min_width)
		size.x = min_width;

	SetOnCharHandler(true);
}

MenuStrip::~MenuStrip()
{
}

void MenuStrip::Draw(ControlDrawData*)
{
	Box2d area = Box2d::Create(global_pos, size);
	gui->DrawArea(area, layout->background);

	Vec2 item_size((float)size.x - (layout->padding.x) * 2,
		(float)layout->font->height + layout->item_padding.y * 2);
	area.v1 = Vec2(global_pos + layout->padding);
	area.v2 = area.v1 + item_size;
	float offset = item_size.y;
	Rect r;

	for(Item& item : items)
	{
		if(item.hover)
			gui->DrawArea(area, layout->button_hover);

		Color color;
		if(!item.enabled)
			color = layout->font_color_disabled;
		else if(item.hover)
			color = layout->font_color_hover;
		else
			color = layout->font_color;
		r = Rect(area, layout->item_padding);
		gui->DrawText(layout->font, item.text, DTF_LEFT, color, r);

		area += Vec2(0, offset);
	}
}

void MenuStrip::OnChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = tolower(c);

	bool first = true;
	int start_index;
	if(selected)
		start_index = selected->index;
	else
		start_index = 0;
	int index = start_index;

	while(true)
	{
		auto& item = items[index];
		char starts_with = tolower(item.text[0]);
		if(starts_with == c)
		{
			if(index == start_index && selected && first)
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
		else if(index == start_index)
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
	if(!mouse_focus)
	{
		if(!focus)
		{
			if(selected)
				selected->hover = false;
			selected = nullptr;
		}
		return;
	}

	Box2d area = Box2d::Create(global_pos, size);
	if(!area.IsInside(gui->cursor_pos))
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

	Vec2 item_size((float)size.x - (layout->padding.x) * 2,
		(float)layout->font->height + layout->item_padding.y * 2);
	area.v1 = Vec2(global_pos + layout->padding);
	area.v2 = area.v1 + item_size;
	float offset = item_size.y;

	for(Item& item : items)
	{
		if(area.IsInside(gui->cursor_pos))
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
						handler(item.index);
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
		if(parent_menu_bar)
			parent_menu_bar->ChangeMenu(-1);
	}
	else if(input->PressedRelease(Key::Right))
	{
		if(parent_menu_bar)
			parent_menu_bar->ChangeMenu(+1);
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
	global_pos = pos;
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
