#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "TextBox.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
class GetNumberDialog : public DialogBox
{
public:
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

	static GetNumberDialog* Show(Control* parent, DialogEvent event, cstring text, int min_value, int max_value, int* value);

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	explicit GetNumberDialog(const DialogInfo& info);

	static GetNumberDialog* self;
	int min_value, max_value;
	int* value;
	TextBox textBox;
	Scrollbar scrollbar;
};
