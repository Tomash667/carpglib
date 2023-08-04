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

	int boxHeight = layout->box.size.y;
	int boxX = globalPos.x + 2;
	int boxY = globalPos.y + (rowHeight - boxHeight) / 2;
	int textX = boxX + layout->box.size.x + 2;
	int textY = globalPos.y + (rowHeight - layout->font->height) / 2;

	Box2d r;
	Rect re;
	int offset = 0;
	for(auto item : items)
	{
		r.v1 = Vec2((float)boxX, (float)boxY + offset);
		r.v2 = r.v1 + Vec2(layout->box.size);
		gui->DrawArea(r, item->checked ? layout->checked : layout->box);

		re = Rect(textX, textY, globalPos.x + size.x - 2, textY + offset + 50);
		gui->DrawText(layout->font, item->name, DTF_LEFT | DTF_SINGLELINE, layout->fontColor, re);

		offset += rowHeight;
	}
}

void CheckBoxGroup::Update(float dt)
{
	if(!mouseFocus)
		return;

	const Int2& boxSize = layout->box.size;
	int boxX = globalPos.x + 2;
	int boxY = globalPos.y + (rowHeight - boxSize.y) / 2;

	int offset = 0;
	for(auto item : items)
	{
		if(Rect::IsInside(gui->cursorPos, boxX, boxY + offset, boxX + boxSize.x, boxY + offset + boxSize.y) && input->Pressed(Key::LeftButton))
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
