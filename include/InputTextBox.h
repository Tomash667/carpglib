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
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;
	// from OnCharHandler
	void OnChar(char c) override;

	void Init();
	void Reset(bool cache = false);
	void Add(Cstring str);
	void CheckLines();

	int max_lines, max_cache;
	InputEvent event;
	Color background_color;
	bool lose_focus, esc_clear;

private:
	Scrollbar scrollbar;
	vector<TextLine> lines;
	vector<string> cache;
	string text, input_str;
	Int2 textbox_size, inputbox_size, inputbox_pos;
	float caret_blink;
	int input_counter, last_input_counter;
	bool added;
};
