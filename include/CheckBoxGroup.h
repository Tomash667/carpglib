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
		Color fontColor;
	};
}

//-----------------------------------------------------------------------------
class CheckBoxGroup : public Control, public LayoutControl<layout::CheckBoxGroup>
{
public:
	CheckBoxGroup();
	~CheckBoxGroup();

	void Add(cstring name, int value);
	void Draw() override;
	int GetValue();
	void SetValue(int value);
	void Update(float dt) override;

private:
	struct Item
	{
		string name;
		int value;
		bool checked;
	};

	vector<Item*> items;
	Scrollbar scrollbar;
	int rowHeight;
};
