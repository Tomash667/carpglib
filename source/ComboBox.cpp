#include "Pch.h"
#include "ComboBox.h"

//=================================================================================================
ComboBox::ComboBox() : menuChanged(false), selected(-1)
{
	menu.eventHandler = delegate<void(int)>(this, &ComboBox::OnSelect);
}

//=================================================================================================
ComboBox::~ComboBox()
{
	if(destructor)
	{
		for(GuiElement* e : menu.items)
			destructor(e);
		menu.items.clear();
	}
}

//=================================================================================================
void ComboBox::Draw()
{
	TextBox::Draw();
	if(menu.visible)
		menu.Draw();
}

//=================================================================================================
void ComboBox::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_GainFocus:
		menuChanged = true;
		break;
	case GuiEvent_LostFocus:
		menu.visible = false;
		break;
	}
	TextBox::Event(e);
}

//=================================================================================================
void ComboBox::Update(float dt)
{
	TextBox::Update(dt);
	if(menuChanged)
	{
		if(menu.items.empty())
			menu.visible = false;
		else
		{
			menu.Show();
			menu.globalPos = menu.pos = globalPos - Int2(0, menu.size.y);
		}
		menuChanged = false;
	}
	if(menu.visible)
	{
		menu.mouseFocus = focus;
		menu.focus = true;
		menu.Update(dt);
		if(!menu.focus)
			LostFocus();
	}
}

//=================================================================================================
void ComboBox::OnTextChanged()
{
	parent->Event((GuiEvent)Event_TextChanged);
}

//=================================================================================================
void ComboBox::Reset()
{
	TextBox::Reset();
	menu.visible = false;
	ClearItems();
	menuChanged = false;
}

//=================================================================================================
void ComboBox::ClearItems()
{
	if(!menu.items.empty())
	{
		menuChanged = true;
		if(destructor)
		{
			for(GuiElement* e : menu.items)
				destructor(e);
		}
		menu.items.clear();
	}
}

//=================================================================================================
void ComboBox::AddItem(GuiElement* e)
{
	menuChanged = true;
	menu.AddItem(e);
}

//=================================================================================================
void ComboBox::OnSelect(int index)
{
	selected = index;
	LostFocus();
	parent->Event((GuiEvent)Event_Selected);
}

//=================================================================================================
GuiElement* ComboBox::GetSelectedItem()
{
	return menu.items[selected];
}
