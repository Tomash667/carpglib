#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "TextBox.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct GetNumberDialogParams
{
	GetNumberDialogParams(int& value, int minValue, int maxValue) : parent(nullptr), event(nullptr), text(nullptr), value(&value), minValue(minValue),
		maxValue(maxValue) {}

	Control* parent;
	DialogEvent event;
	delegate<cstring()> getText;
	cstring text;
	int* value;
	int minValue, maxValue;
};

//-----------------------------------------------------------------------------
class GetNumberDialog : public DialogBox
{
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

public:
	static GetNumberDialog* Show(const GetNumberDialogParams& params);
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	explicit GetNumberDialog(const DialogInfo& info);

	static GetNumberDialog* self;
	delegate<cstring()> getText;
	int minValue, maxValue;
	int* value;
	TextBox textBox;
	Scrollbar scrollbar;
};
