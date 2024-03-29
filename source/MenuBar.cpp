#include "Pch.h"
#include "MenuBar.h"

#include "Input.h"
#include "MenuStrip.h"

MenuBar::MenuBar() : selected(nullptr), handler(nullptr)
{
}

MenuBar::~MenuBar()
{
	DeleteElements(items);
}

void MenuBar::Draw()
{
	// backgroud
	gui->DrawArea(rect, layout->background);

	// items
	for(Item* item : items)
	{
		AreaLayout* areaLayout;
		Color fontColor;
		switch(item->mode)
		{
		case Item::Up:
		default:
			areaLayout = &layout->button;
			fontColor = layout->fontColor;
			break;
		case Item::Hover:
			areaLayout = &layout->buttonHover;
			fontColor = layout->fontColorHover;
			break;
		case Item::Down:
			areaLayout = &layout->buttonDown;
			fontColor = layout->fontColorDown;
			break;
		}

		// item background
		gui->DrawArea(item->rect, *areaLayout);

		// item text
		gui->DrawText(layout->font, item->text, DTF_CENTER | DTF_VCENTER, fontColor, Rect(item->rect));
	}
}

void MenuBar::Update(float dt)
{
	bool down = false;
	if(selected)
	{
		if(selected->mode != Item::Down)
			selected->mode = Item::Up;
		else
			down = true;
	}

	if(!mouseFocus || !rect.IsInside(gui->cursorPos))
		return;

	for(Item* item : items)
	{
		if(item->rect.IsInside(gui->cursorPos))
		{
			if(down || input->Pressed(Key::LeftButton))
			{
				if((item != selected || item->mode != Item::Down) && (input->Pressed(Key::LeftButton) || gui->MouseMoved()))
				{
					EnsureMenu(item);
					item->menu->ShowMenu(Int2(item->rect.LeftBottom()));
					selected = item;
					selected->mode = Item::Down;
				}
			}
			else
			{
				selected = item;
				selected->mode = Item::Hover;
			}
			break;
		}
	}
}

void MenuBar::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Initialize:
		// control initialization, menubar must be attacked to other control (like Window)
		Update(true, true);
		break;
	case GuiEvent_Moved:
		// parent control moved
		Update(true, false);
		break;
	case GuiEvent_Resize:
		// parent window resized
		Update(false, true);
		break;
	case GuiEvent_Show:
		// control is shown, forget old selected item
		selected = nullptr;
		for(Item* item : items)
			item->mode = Item::Up;
		break;
	}
}

void MenuBar::AddMenu(cstring text, std::initializer_list<SimpleMenuCtor> const & _items)
{
	assert(text);

	float itemHeight = (float)layout->font->height + layout->itemPadding.y * 2;
	float itemWidth = (float)layout->font->CalculateSize(text).x + layout->itemPadding.x * 2;

	Item* item = new Item;
	item->text = text;
	item->rect = Box2d(0, 0, itemWidth, itemHeight);
	item->rect += Vec2(layout->padding) / 2;
	if(!items.empty())
		item->rect += Vec2(items.back()->rect.v2.x, 0);
	item->index = items.size();
	item->items = _items;
	items.push_back(item);
}

void MenuBar::Update(bool move, bool resize)
{
	assert(parent);
	Int2 prevPos = globalPos;
	if(move)
		globalPos = parent->globalPos;
	if(resize)
		size = Int2(parent->size.x, layout->font->height + layout->padding.y + layout->itemPadding.y * 2);
	rect = Box2d::Create(globalPos, size);
	if(move)
	{
		Vec2 offset = Vec2(globalPos - prevPos);
		for(Item* item : items)
			item->rect += offset;
	}
}

void MenuBar::OnCloseMenuStrip()
{
	if(selected)
		selected->mode = Item::Up;
}

// offset must be -1/+1, menu must be open
void MenuBar::ChangeMenu(int offset)
{
	assert(offset == -1 || offset == 1);
	assert(selected);

	bool prev = (offset == -1);
	int index = Modulo(selected->index + (prev ? -1 : 1), items.size());
	Item* item = items[index];

	// showing new menu should close the old one
	EnsureMenu(item);
	item->menu->ShowMenu(Int2(item->rect.LeftBottom()));
	item->menu->SetSelectedIndex(0);
	selected = item;
	selected->mode = Item::Down;
}

void MenuBar::EnsureMenu(Item* item)
{
	if(item->menu)
		return;
	MenuStrip* menu = new MenuStrip(item->items, (int)item->rect.SizeX());
	menu->SetOwner(this, item->index);
	menu->SetHandler(handler);
	menu->SetOnCloseHandler(MenuStrip::OnCloseHandler(this, &MenuBar::OnCloseMenuStrip));
	menu->Initialize();
	item->menu = menu;
}
