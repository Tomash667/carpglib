#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct CheckBox : public Control
	{
		AreaLayout tex[4];
		AreaLayout tick;
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class CheckBox : public Control, LayoutControl<layout::CheckBox>
{
public:
	enum State
	{
		NONE,
		HOVER,
		DOWN,
		DISABLED
	};

	CheckBox(Cstring text = "", bool checked = false);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	string text;
	int id;
	State state;
	Int2 bt_size;
	bool checked, radiobox;
};
