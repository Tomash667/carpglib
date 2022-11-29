#include "Pch.h"
#include "Label.h"

Label::Label(cstring text, bool auto_size) : text(text), auto_size(auto_size)
{
	if(auto_size)
		CalculateSize();
}

void Label::Draw()
{
	Rect rect = { globalPos.x, globalPos.y, globalPos.x + size.x, globalPos.y + size.y };
	gui->DrawText(layout->font, text, layout->align, layout->color, rect);
}

void Label::SetText(Cstring s)
{
	text = s.s;
	CalculateSize();
}

void Label::SetSize(const Int2& _size)
{
	assert(!auto_size);
	size = _size;
}

void Label::CalculateSize()
{
	if(!auto_size)
		return;
	size = layout->font->CalculateSize(text) + layout->padding * 2;
}
