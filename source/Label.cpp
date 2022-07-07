#include "Pch.h"
#include "Label.h"

Label::Label(bool autoSize) : text(""), autoSize(autoSize)
{
}

Label::Label(cstring text, bool autoSize) : text(text), autoSize(autoSize)
{
	if(autoSize)
		CalculateSize();
}

void Label::Draw(ControlDrawData*)
{
	Rect rect = { global_pos.x, global_pos.y, global_pos.x + size.x, global_pos.y + size.y };
	gui->DrawText(layout->font, text, 0, layout->color, rect);
}

void Label::SetText(Cstring s)
{
	text = s.s;
	CalculateSize();
}

void Label::SetSize(const Int2& _size)
{
	assert(!autoSize);
	size = _size;
}

void Label::CalculateSize()
{
	if(!autoSize)
		return;
	size = layout->font->CalculateSize(text);
}
