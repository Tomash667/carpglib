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
class CheckBox : public Control, public LayoutControl<layout::CheckBox>
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

	void Draw() override;
	void Update(float dt) override;

	string text;
	int id;
	State state;
	Int2 btSize;
	bool checked, radiobox;
};
