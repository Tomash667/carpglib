#pragma once

//-----------------------------------------------------------------------------
#include "DialogBox.h"
#include "TextBox.h"

//-----------------------------------------------------------------------------
struct GetTextDialogParams
{
	GetTextDialogParams(cstring text, string& inputStr) : text(text), inputStr(&inputStr), parent(nullptr), event(nullptr), validate(nullptr), limit(0),
		lines(1), width(300), customNames(nullptr), multiline(false)
	{
	}

	Control* parent;
	DialogEvent event;
	delegate<bool(const string&)> validate;
	cstring text;
	string* inputStr;
	int limit, lines, width;
	cstring* customNames;
	bool multiline;
};

//-----------------------------------------------------------------------------
class GetTextDialog : public DialogBox
{
	enum Result
	{
		Result_Ok = GuiEvent_Custom,
		Result_Cancel
	};

public:
	static GetTextDialog* Show(const GetTextDialogParams& params);
	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

private:
	explicit GetTextDialog(const DialogInfo& info);
	void Create(const GetTextDialogParams& params);
	void UpdateButtons();

	static GetTextDialog* self;
	string* inputStr;
	string prevText;
	delegate<bool(const string&)> validate;
	TextBox textBox;
	bool singleline;
};
