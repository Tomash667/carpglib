#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "GuiElement.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct MenuList : public Control
	{
		AreaLayout box;
		AreaLayout selection;
		Font* font;
		int border, padding, itemHeight;
	};
}

//-----------------------------------------------------------------------------
class MenuList : public Control, public LayoutControl<layout::MenuList>
{
public:
	MenuList(bool isNew = false);
	~MenuList();

	void AddItem(GuiElement* e);
	void AddItem(cstring text) { AddItem(new DefaultGuiElement(text)); }
	void AddItems(vector<GuiElement*>& items, bool itemsOwner = true);
	void Draw() override;
	void Event(GuiEvent e) override;
	int GetIndex() { return selected; }
	GuiElement* GetItem() { return selected == -1 ? nullptr : items[selected]; }
	void Reset();
	void Select(int index) { selected = index; }
	void Select(delegate<bool(GuiElement*)> pred);
	void Update(float dt) override;

	DialogEvent eventHandler;
	vector<GuiElement*> items;
	int minWidth;

private:
	void PrepareItem(cstring text);

	int width, selected;
	bool itemsOwner, recalculate;
};
