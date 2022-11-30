#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
class Scrollbar;

//-----------------------------------------------------------------------------
namespace layout
{
	struct TextBox : public Control
	{
		AreaLayout background;
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class TextBox : public Control, public LayoutControl<layout::TextBox>, public OnCharHandler
{
public:
	explicit TextBox(bool isNew = false);
	~TextBox();

	// from Control
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;
	// new
	virtual void OnTextChanged() {}

	void AddScrollbar();
	void Move(const Int2& movePos);
	void Add(cstring str);
	void CalculateOffset(bool center);
	void Reset();
	void UpdateScrollbar();
	void UpdateSize(const Int2& pos, const Int2& size);
	void SetText(cstring text);
	const string& GetText() const { return text; }
	void SelectAll();
	bool IsMultiline() const { return multiline; }
	bool IsNumeric() const { return numeric; }
	bool IsReadonly() const { return readonly; }
	void SetMultiline(bool multiline) { assert(!initialized); this->multiline = multiline; }
	void SetNumeric(bool numeric) { this->numeric = numeric; }
	void SetReadonly(bool readonly) { this->readonly = readonly; }

	int limit, numMin, numMax;
	cstring label;
	Scrollbar* scrollbar;

private:
	void ValidateNumber();
	void GetCaretPos(const Int2& inPos, Int2& index, Int2& pos, uint* charIndex = nullptr);
	void CalculateSelection(const Int2& newIndex, const Int2& newPos);
	void CalculateSelection(Int2 index1, Int2 pos1, Int2 index2, Int2 pos2);
	void DeleteSelection();
	Int2 IndexToPos(const Int2& index);
	uint ToRawIndex(const Int2& index);
	void UpdateFontLines();

	string text;
	vector<Font::Line> fontLines;
	Int2 realSize, textSize, caretPos, caretIndex, selectStartPos, selectEndPos, selectStartIndex, selectEndIndex, selectFixedIndex;
	float caretBlink, offsetMove;
	int offset, lastYMove;
	bool added, down, readonly, multiline, numeric, requireScrollbar;
};
