#include "Pch.h"
#include "Label.h"

Label::Label(cstring text, bool autoSize) : text(text), autoSize(autoSize)
{
	if(autoSize)
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
	assert(!autoSize);
	size = _size;
}

void Label::CalculateSize()
{
	if(!autoSize)
		return;
	size = layout->font->CalculateSize(text) + layout->padding * 2;
}
