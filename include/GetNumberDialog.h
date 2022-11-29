#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "TextBox.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
class GetNumberDialog : public DialogBox
{
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

public:
	static GetNumberDialog* Show(Control* parent, DialogEvent event, cstring text, int minValue, int maxValue, int* value);
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	explicit GetNumberDialog(const DialogInfo& info);

	static GetNumberDialog* self;
	int minValue, maxValue;
	int* value;
	TextBox textBox;
	Scrollbar scrollbar;
};
