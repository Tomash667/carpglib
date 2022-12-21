#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct InputTextBox : public Control
	{
		AreaLayout background;
		AreaLayout box;
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class InputTextBox : public Control, public LayoutControl<layout::InputTextBox>, public OnCharHandler
{
public:
	typedef delegate<void(const string&)> InputEvent;

	InputTextBox();

	// from Control
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

	void Init();
	void Reset(bool resetCache = false);
	void Add(Cstring str);
	void CheckLines();

	string text, inputStr;
	vector<TextLine> lines;
	vector<string> cache;
	int maxLines, maxCache, inputCounter, lastInputCounter;
	Scrollbar scrollbar;
	Int2 textboxSize, inputboxSize, inputboxPos;
	InputEvent event;
	Color backgroundColor;
	bool added, loseFocus, escClear;

private:
	float caretBlink;
};
