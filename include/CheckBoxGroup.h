#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct CheckBoxGroup : public Control
	{
		AreaLayout background;
		AreaLayout box;
		AreaLayout checked;
		Font* font;
		Color font_color;
	};
}

//-----------------------------------------------------------------------------
class CheckBoxGroup : public Control, public LayoutControl<layout::CheckBoxGroup>
{
public:
	CheckBoxGroup();
	~CheckBoxGroup();

	void Draw(ControlDrawData*) override;
	void Update(float dt) override;

	void Add(cstring name, int value);
	int GetValue();
	void SetValue(int value);

private:
	struct Item
	{
		string name;
		int value;
		bool checked;
	};

	vector<Item*> items;
	Scrollbar scrollbar;
	int row_height;
};
