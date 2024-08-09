#include "Pch.h"
#include "MenuList.h"

#include "Input.h"

//=================================================================================================
MenuList::MenuList(bool isNew) : Control(isNew), eventHandler(nullptr), width(0), minWidth(0), selected(-1), itemsOwner(true), recalculate(false)
{
}

//=================================================================================================
MenuList::~MenuList()
{
	if(itemsOwner)
		DeleteElements(items);
}

//=================================================================================================
void MenuList::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size), layout->box);

	if(selected != -1)
	{
		Rect r2 = { globalPos.x + layout->border, globalPos.y + layout->border + selected * layout->itemHeight,
			globalPos.x + size.x - layout->border, globalPos.y + layout->border + (selected + 1) * layout->itemHeight };
		gui->DrawArea(Box2d(r2), layout->selection);
	}

	Rect rect = { globalPos.x + layout->border + layout->padding, globalPos.y + layout->border,
		globalPos.x + size.x - layout->border - layout->padding, globalPos.y + layout->border + layout->itemHeight };
	for(GuiElement* e : items)
	{
		gui->DrawText(layout->font, e->ToString(), DTF_SINGLELINE, Color::Black, rect, &rect);
		rect.Top() += layout->itemHeight;
		rect.Bottom() += layout->itemHeight;
	}
}

//=================================================================================================
void MenuList::Update(float dt)
{
	selected = -1;
	if(IsInside(gui->cursorPos))
	{
		selected = (gui->cursorPos.y - globalPos.y - layout->border) / layout->itemHeight;
		if(selected >= (int)items.size())
			selected = -1;
	}

	if(input->PressedRelease(Key::LeftButton))
	{
		if(selected != -1 && eventHandler)
			eventHandler(selected);
		LostFocus();
	}
	else if(input->PressedRelease(Key::RightButton) || input->PressedRelease(Key::Escape))
		LostFocus();
}

//=================================================================================================
void MenuList::Event(GuiEvent e)
{
	if(e == GuiEvent_LostFocus)
		visible = false;
	else if(e == GuiEvent_Show)
{
		focus = true;
		if(recalculate)
		{
			size = Int2(width + (layout->border + layout->padding) * 2, layout->border * 2 + layout->itemHeight * items.size());
			if(size.x < minWidth)
				size.x = minWidth;
			recalculate = false;
}
	}
}

//=================================================================================================
void MenuList::Reset()
{
	items.clear();
	size.y = layout->border * 2;
}

//=================================================================================================
void MenuList::AddItem(GuiElement* e)
{
	assert(e);
	items.push_back(e);
	PrepareItem(e->ToString());
	recalculate = true;
}

//=================================================================================================
void MenuList::AddItems(vector<GuiElement*>& newItems, bool itemsOwner)
{
	this->itemsOwner = itemsOwner;
	for(GuiElement* e : newItems)
	{
		assert(e);
		items.push_back(e);
		PrepareItem(e->ToString());
	}
	recalculate = true;
}

//=================================================================================================
void MenuList::PrepareItem(cstring text)
{
	assert(text);
	const int itemWidth = layout->font->CalculateSize(text).x;
	if(itemWidth > width)
		width = itemWidth;
}

//=================================================================================================
void MenuList::Select(delegate<bool(GuiElement*)> pred)
{
	int index = 0;
	for(GuiElement* item : items)
	{
		if(pred(item))
		{
			selected = index;
			break;
		}
		++index;
	}
}
