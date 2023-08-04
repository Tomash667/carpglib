#include "Pch.h"
#include "GetTextDialog.h"

#include "Input.h"

//-----------------------------------------------------------------------------
GetTextDialog* GetTextDialog::self;

//=================================================================================================
GetTextDialog::GetTextDialog(const DialogInfo& info) : DialogBox(info), singleline(true)
{
}

//=================================================================================================
void GetTextDialog::Draw()
{
	gui->DrawArea(Box2d::Create(Int2::Zero, gui->wndSize), layout->background);
	gui->DrawArea(Box2d::Create(globalPos, size), layout->box);

	for(int i = 0; i < 2; ++i)
		bts[i].Draw();

	Rect r = { globalPos.x + 16, globalPos.y + 16, globalPos.x + size.x - 16, globalPos.y + 60 - 16 };
	gui->DrawText(layout->font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r);

	textBox.Draw();
}

//=================================================================================================
void GetTextDialog::Update(float dt)
{
	textBox.mouseFocus = focus;

	if(input->Focus() && focus)
	{
		for(int i = 0; i < 2; ++i)
		{
			bts[i].mouseFocus = focus;
			bts[i].Update(dt);
		}

		textBox.focus = true;
		textBox.Update(dt);

		if(prevText != textBox.GetText())
		{
			prevText = textBox.GetText();
			UpdateButtons();
		}

		if(result == -1)
		{
			if(input->PressedRelease(Key::Escape))
				result = BUTTON_CANCEL;
			else if(input->PressedRelease(Key::Enter))
			{
				if(!textBox.IsMultiline() || input->Down(Key::Shift))
				{
					input->SetState(Key::Enter, Input::IS_UP);
					result = BUTTON_OK;
				}
			}
		}

		if(result != -1)
		{
			if(result == BUTTON_OK)
			{
				*inputStr = textBox.GetText();
				if(validate && !validate())
				{
					textBox.SetText(*inputStr);
					textBox.SetErrorMode(true);
					result = -1;
					return;
				}
			}
			gui->CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void GetTextDialog::Event(GuiEvent e)
{
	switch(e)
	{
	case GuiEvent_Show:
		textBox.SetErrorMode(false);
		break;
	case GuiEvent_GainFocus:
		textBox.focus = true;
		textBox.Event(GuiEvent_GainFocus);
		break;
	case GuiEvent_LostFocus:
	case GuiEvent_Close:
		textBox.focus = false;
		textBox.Event(GuiEvent_LostFocus);
		break;
	case GuiEvent_WindowResize:
		self->pos = self->global_pos = (gui->wndSize - self->size) / 2;
		self->bts[0].global_pos = self->bts[0].pos + self->global_pos;
		self->bts[1].global_pos = self->bts[1].pos + self->global_pos;
		self->textBox.global_pos = self->textBox.pos + self->global_pos;
		break;
	default:
		if(e >= GuiEvent_Custom)
		{
			if(e == Result_Ok)
				result = BUTTON_OK;
			else if(e == Result_Cancel)
				result = BUTTON_CANCEL;
		}
		break;
	}
}

//=================================================================================================
GetTextDialog* GetTextDialog::Show(const GetTextDialogParams& params)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "GetTextDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = DialogOrder::Normal;
		info.type = DialogType::Custom;

		self = new GetTextDialog(info);
		self->bts.resize(2);

		Button& bt1 = self->bts[0],
			&bt2 = self->bts[1];

		bt1.id = Result_Cancel;
		bt1.size = Int2(100, 40);
		bt1.parent = self;

		bt2.id = Result_Ok;
		bt2.size = Int2(100, 40);
		bt2.parent = self;

		self->textBox.pos = Int2(25, 60);

		gui->RegisterControl(self);
	}

	self->Create(params);

	gui->ShowDialog(self);

	return self;
}

//=================================================================================================
void GetTextDialog::Create(const GetTextDialogParams& params)
{
	Button& bt1 = bts[0],
		&bt2 = bts[1];

	int lines = params.lines;
	if(!params.multiline || params.lines < 1)
		lines = 1;

	size = Int2(params.width, 140 + lines * 20);
	textBox.size = Int2(params.width - 50, 15 + lines * 20);
	textBox.SetMultiline(params.multiline);
	textBox.limit = params.limit;
	textBox.SetText(params.inputStr->c_str());

	// ustaw przyciski
	bt1.pos = Int2(size.x - 100 - 16, size.y - 40 - 16);
	bt2.pos = Int2(16, size.y - 40 - 16);
	if(params.customNames)
	{
		bt1.text = (params.customNames[0] ? params.customNames[0] : gui->txCancel);
		bt2.text = (params.customNames[1] ? params.customNames[1] : gui->txOk);
	}
	else
	{
		bt1.text = gui->txCancel;
		bt2.text = gui->txOk;
	}

	// ustaw parametry
	result = -1;
	parent = params.parent;
	order = GetOrder(params.parent);
	event = params.event;
	validate = params.validate;
	text = params.text;
	inputStr = params.inputStr;
	prevText = *inputStr;
	UpdateButtons();

	// ustaw pozycjê
	pos = globalPos = (gui->wndSize - size) / 2;
	bt1.globalPos = bt1.pos + globalPos;
	bt2.globalPos = bt2.pos + globalPos;
	textBox.globalPos = textBox.pos + globalPos;
}

//=================================================================================================
void GetTextDialog::UpdateButtons()
{
	const string& text = textBox.GetText();
	bool ok;
	if(validate)
		ok = validate(text);
	else
		ok = !text.empty();

	if(ok)
	{
		if(bts[1].state == Button::DISABLED)
			bts[1].state = Button::NONE;
	}
	else
		bts[1].state = Button::DISABLED;
}
