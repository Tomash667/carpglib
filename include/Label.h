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
	explicit Label(bool autoSize = true);
	explicit Label(cstring text, bool autoSize = true);
	void Draw() override;
	const string& GetText() const { return text; }
	bool IsAutoSize() const { return autoSize; }
	void SetText(Cstring s);
	void SetSize(const Int2& size);

private:
	void CalculateSize();

	string text;
	bool autoSize;
};
