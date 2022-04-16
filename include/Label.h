#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Label : public Control
	{
		Font* font;
		Color color;
	};
}

//-----------------------------------------------------------------------------
class Label : public Control, public LayoutControl<layout::Label>
{
public:
	Label(bool auto_size = true);
	Label(cstring text, bool auto_size = true);
	void Draw(ControlDrawData*) override;
	const string& GetText() const { return text; }
	bool IsAutoSize() const { return auto_size; }
	void SetText(Cstring s);
	void SetSize(const Int2& size);

private:
	void CalculateSize();

	string text;
	bool auto_size, own_custom_layout;
};
