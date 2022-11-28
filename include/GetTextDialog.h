#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
struct GetTextDialogParams
{
	GetTextDialogParams(cstring text, string& input_str) : text(text), input_str(&input_str), parent(nullptr), event(nullptr), limit(0), lines(1), width(300),
		custom_names(nullptr), multiline(false)
	{
	}

	Control* parent;
	DialogEvent event;
	cstring text;
	string* input_str;
	int limit, lines, width;
	cstring* custom_names;
	bool multiline;
};

//-----------------------------------------------------------------------------
class GetTextDialog : public DialogBox
{
public:
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

	static GetTextDialog* Show(const GetTextDialogParams& params);
	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	explicit GetTextDialog(const DialogInfo& info);
	void Create(const GetTextDialogParams& params);

	static GetTextDialog* self;
	string* input_str;
	TextBox textBox;
	bool singleline;
};
