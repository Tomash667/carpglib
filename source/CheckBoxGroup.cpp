#include "Pch.h"
#include "CheckBoxGroup.h"

#include "Input.h"

CheckBoxGroup::CheckBoxGroup() : scrollbar(false, true)
{
	rowHeight = max(layout->font->height, layout->box.size.y) + 2;
}

CheckBoxGroup::~CheckBoxGroup()
{
	DeleteElements(items);
}

void CheckBoxGroup::Draw()
{
	Box2d rect = Box2d::Create(globalPos, size);
	gui->DrawArea(rect, layout->background);

	int box_height = layout->box.size.y;
	int box_x = globalPos.x + 2;
	int box_y = globalPos.y + (rowHeight - box_height) / 2;
	int text_x = box_x + layout->box.size.x + 2;
	int text_y = globalPos.y + (rowHeight - layout->font->height) / 2;

	Box2d r;
	Rect re;
	int offset = 0;
	for(auto item : items)
	{
		r.v1 = Vec2((float)box_x, (float)box_y + offset);
		r.v2 = r.v1 + Vec2(layout->box.size);
		gui->DrawArea(r, item->checked ? layout->checked : layout->box);

		re = Rect(text_x, text_y, globalPos.x + size.x - 2, text_y + offset + 50);
		gui->DrawText(layout->font, item->name, DTF_LEFT | DTF_SINGLELINE, layout->font_color, re);

		offset += rowHeight;
	}
}

void CheckBoxGroup::Update(float dt)
{
	if(!mouseFocus)
		return;

	const Int2& box_size = layout->box.size;
	int box_x = globalPos.x + 2;
	int box_y = globalPos.y + (rowHeight - box_size.y) / 2;

	int offset = 0;
	for(auto item : items)
	{
		if(Rect::IsInside(gui->cursorPos, box_x, box_y + offset, box_x + box_size.x, box_y + offset + box_size.y) && input->Pressed(Key::LeftButton))
		{
			item->checked = !item->checked;
			TakeFocus(true);
			break;
		}

		offset += rowHeight;
	}
}

void CheckBoxGroup::Add(cstring name, int value)
{
	assert(name);

	auto item = new Item;
	item->name = name;
	item->value = value;
	item->checked = false;

	items.push_back(item);
}

int CheckBoxGroup::GetValue()
{
	int value = 0;

	for(auto item : items)
	{
		if(item->checked)
			value |= item->value;
	}

	return value;
}

void CheckBoxGroup::SetValue(int value)
{
	for(auto item : items)
		item->checked = IsSet(value, item->value);
}
