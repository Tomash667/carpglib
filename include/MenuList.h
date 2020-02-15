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
	};
}

//-----------------------------------------------------------------------------
class MenuList : public Control, public LayoutControl<layout::MenuList>
{
public:
	MenuList();
	~MenuList();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void Init();
	void AddItem(GuiElement* e);
	void AddItem(cstring text) { AddItem(new DefaultGuiElement(text)); }
	void AddItems(vector<GuiElement*>& items, bool items_owner = true);
	int GetIndex() { return selected; }
	GuiElement* GetItem() { return selected == -1 ? nullptr : items[selected]; }
	void PrepareItem(cstring text);
	void Select(delegate<bool(GuiElement*)> pred);
	void Select(int index) { selected = index; }

	DialogEvent event_handler;
	vector<GuiElement*> items;

private:
	int w, selected;
	bool items_owner;
};
