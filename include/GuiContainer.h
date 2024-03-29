#pragma once

//-----------------------------------------------------------------------------
#include "Control.h"

//-----------------------------------------------------------------------------
class GuiContainer : public Control
{
public:
	enum Flags
	{
		F_MOUSE_FOCUS = 1 << 0,
		F_FOCUS = 1 << 1,
		F_CLICK_TO_FOCUS = 1 << 2
	};

	typedef pair<Control*, int> GuiItem;
	typedef vector<GuiItem>::iterator Iter;

	GuiContainer();

	void Draw();
	void Update(float dt);
	void Add(Control* ctrl, int flags)
	{
		assert(ctrl);
		items.push_back(GuiItem(ctrl, flags));
	}
	void Add(TextBox* textbox) { Add((Control*)textbox, F_FOCUS | F_CLICK_TO_FOCUS | F_MOUSE_FOCUS); }
	void Add(TextBox& textbox) { Add(&textbox); }
	void Add(Button* button) { Add((Control*)button, F_MOUSE_FOCUS); }
	void Add(Button& button) { Add(&button); }
	void Add(CheckBox* checkbox) { Add((Control*)checkbox, F_MOUSE_FOCUS); }
	void Add(CheckBox& checkbox) { Add(&checkbox); }
	void GainFocus();
	void LostFocus();
	void Move(const Int2& globalPos);
	void CheckGiveFocus();

	vector<GuiItem> items;
	Control* focusCtrl, *giveFocus;
	bool focus;
};
