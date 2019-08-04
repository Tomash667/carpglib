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

	string text, input_str;
	vector<TextLine> lines;
	vector<string> cache;
	int max_lines, max_cache, input_counter, last_input_counter;
	Scrollbar scrollbar;
	Int2 textbox_size, inputbox_size, inputbox_pos;
	InputEvent event;
	Color background_color;
	bool added, lose_focus, esc_clear;

private:
	float caret_blink;
};
