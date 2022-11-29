#pragma once

//-----------------------------------------------------------------------------
#include "TextBox.h"
#include "MenuList.h"

//-----------------------------------------------------------------------------
class ComboBox : public TextBox
{
public:
	enum Id
	{
		Event_TextChanged = GuiEvent_Custom,
		Event_Selected
	};

	ComboBox();
	~ComboBox();

	void AddItem(GuiElement* e);
	void ClearItems();
	void Draw() override;
	void Event(GuiEvent e) override;
	GuiElement* GetSelectedItem();
	void OnSelect(int index);
	void OnTextChanged() override;
	void Reset();
	void Update(float dt) override;

	delegate<void(GuiElement*)> destructor;

private:
	MenuList menu;
	int selected;
	bool menuChanged;
};
