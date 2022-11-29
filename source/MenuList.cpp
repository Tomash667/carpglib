#include "Pch.h"
#include "MenuList.h"

#include "Input.h"

//=================================================================================================
MenuList::MenuList(bool is_new) : Control(is_new), event_handler(nullptr), w(0), selected(-1), items_owner(true)
{
}

//=================================================================================================
MenuList::~MenuList()
{
	if(items_owner)
		DeleteElements(items);
}

//=================================================================================================
void MenuList::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size), layout->box);

	if(selected != -1)
	{
		Rect r2 = { globalPos.x + 4, globalPos.y + 4 + selected * 20, globalPos.x + size.x - 4, globalPos.y + 24 + selected * 20 };
		gui->DrawArea(Box2d(r2), layout->selection);
	}

	Rect rect = { globalPos.x + 5, globalPos.y + 5, globalPos.x + size.x - 5, globalPos.y + 25 };
	for(GuiElement* e : items)
	{
		gui->DrawText(layout->font, e->ToString(), DTF_SINGLELINE, Color::Black, rect, &rect);
		rect.Top() += 20;
		rect.Bottom() += 20;
	}
}

//=================================================================================================
void MenuList::Update(float dt)
{
	selected = -1;
	if(IsInside(gui->cursorPos))
	{
		selected = (gui->cursorPos.y - globalPos.y) / 20;
		if(selected >= (int)items.size())
			selected = -1;
	}
	if(input->Focus())
	{
		if(input->PressedRelease(Key::LeftButton))
		{
			if(selected != -1 && event_handler)
				event_handler(selected);
			LostFocus();
		}
		else if(input->PressedRelease(Key::RightButton) || input->PressedRelease(Key::Escape))
			LostFocus();
	}
}

//=================================================================================================
void MenuList::Event(GuiEvent e)
{
	if(e == GuiEvent_LostFocus)
		visible = false;
}

//=================================================================================================
void MenuList::Init()
{
	size = Int2(w + 10, 20 * items.size() + 10);
}

//=================================================================================================
void MenuList::Reset()
{
	items.clear();
	size.y = 10;
}

//=================================================================================================
void MenuList::AddItem(GuiElement* e)
{
	assert(e);
	items.push_back(e);
	PrepareItem(e->ToString());
	size.y += 20;
}

//=================================================================================================
void MenuList::AddItems(vector<GuiElement*>& new_items, bool is_owner)
{
	items_owner = is_owner;
	for(GuiElement* e : new_items)
	{
		assert(e);
		items.push_back(e);
		PrepareItem(e->ToString());
	}
}

//=================================================================================================
void MenuList::PrepareItem(cstring text)
{
	assert(text);
	int w2 = layout->font->CalculateSize(text).x;
	if(w2 > w)
		w = w2;
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
