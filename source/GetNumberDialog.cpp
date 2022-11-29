#include "Pch.h"
#include "GetNumberDialog.h"

#include "Input.h"

//-----------------------------------------------------------------------------
GetNumberDialog* GetNumberDialog::self;

//=================================================================================================
GetNumberDialog::GetNumberDialog(const DialogInfo& info) : DialogBox(info), scrollbar(true)
{
}

//=================================================================================================
void GetNumberDialog::Draw()
{
	gui->DrawArea(Box2d::Create(Int2::Zero, gui->wndSize), layout->background);
	gui->DrawArea(Box2d::Create(globalPos, size), layout->box);

	for(int i = 0; i < 2; ++i)
		bts[i].Draw();

	Rect r = { globalPos.x + 16, globalPos.y + 16, globalPos.x + size.x - 16, globalPos.y + 60 - 16 };
	gui->DrawText(layout->font, text, DTF_CENTER | DTF_VCENTER, Color::Black, r);

	textBox.Draw();
	scrollbar.Draw();

	Rect r2 = { globalPos.x + 16, globalPos.y + 120, globalPos.x + size.x - 16, globalPos.y + size.y };
	gui->DrawText(layout->font, Format("%d", minValue), DTF_LEFT, Color::Black, r2);
	gui->DrawText(layout->font, Format("%d", maxValue), DTF_RIGHT, Color::Black, r2);
}

//=================================================================================================
void GetNumberDialog::Update(float dt)
{
	textBox.mouseFocus = focus;

	if(input->Focus() && focus)
	{
		for(int i = 0; i < 2; ++i)
		{
			bts[i].mouseFocus = focus;
			bts[i].Update(dt);
		}

		const float wheel = input->GetMouseWheel();
		bool moving = scrollbar.clicked;
		float prevOffset = scrollbar.offset;
		scrollbar.ApplyMouseWheel();
		scrollbar.mouseFocus = focus;
		scrollbar.Update(dt);
		bool changed = false;
		if(scrollbar.change != 0 || wheel != 0.f)
		{
			int num = atoi(textBox.GetText().c_str());
			int newNum = num;

			if(wheel != 0.f)
			{
				int change = 1;
				if(input->Down(Key::Shift))
					change = max(1, (maxValue - minValue) / 20);
				if(wheel < 0.f)
					change = -change;
				newNum += change;
			}
			else
				newNum += scrollbar.change;

			newNum = Clamp(newNum, minValue, maxValue);
			if(num != newNum)
			{
				textBox.SetText(Format("%d", newNum));
				scrollbar.offset = float(newNum - minValue) / (maxValue - minValue) * (scrollbar.total - scrollbar.part);
				changed = true;
			}
		}
		else if(!Equal(scrollbar.offset, prevOffset))
		{
			const float progress = scrollbar.offset / (scrollbar.total - scrollbar.part);
			const int value = (int)round((maxValue - minValue) * progress) + minValue;
			textBox.SetText(Format("%d", value));
			changed = true;
		}

		if(moving)
		{
			if(!scrollbar.clicked)
			{
				textBox.focus = true;
				textBox.Event(GuiEvent_GainFocus);
			}
			else
				changed = true;
		}
		else if(scrollbar.clicked)
		{
			textBox.focus = false;
			textBox.Event(GuiEvent_LostFocus);
		}

		textBox.Update(dt);
		if(!changed && !textBox.GetText().empty())
		{
			int num = atoi(textBox.GetText().c_str());
			if(!scrollbar.clicked && minValue != maxValue)
				scrollbar.offset = float(num - minValue) / (maxValue - minValue) * (scrollbar.total - scrollbar.part);
		}
		if(textBox.GetText().empty())
			bts[1].state = Button::DISABLED;
		else if(bts[1].state == Button::DISABLED)
			bts[1].state = Button::NONE;

		if(result == -1)
		{
			if(input->PressedRelease(Key::Escape))
				result = BUTTON_CANCEL;
			else if(input->PressedRelease(Key::Enter))
				result = BUTTON_OK;
		}

		if(result != -1)
		{
			if(result == BUTTON_OK)
				*value = atoi(textBox.GetText().c_str());
			gui->CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void GetNumberDialog::Event(GuiEvent e)
{
	if(e == GuiEvent_GainFocus)
	{
		textBox.focus = true;
		textBox.Event(GuiEvent_GainFocus);
	}
	else if(e == GuiEvent_LostFocus || e == GuiEvent_Close)
	{
		textBox.focus = false;
		textBox.Event(GuiEvent_LostFocus);
		scrollbar.LostFocus();
	}
	else if(e == GuiEvent_WindowResize)
	{
		self->pos = self->globalPos = (gui->wndSize - self->size) / 2;
		self->bts[0].globalPos = self->bts[0].pos + self->globalPos;
		self->bts[1].globalPos = self->bts[1].pos + self->globalPos;
		self->textBox.globalPos = self->textBox.pos + self->globalPos;
		self->scrollbar.globalPos = self->scrollbar.pos + self->globalPos;
	}
	else if(e >= GuiEvent_Custom)
	{
		if(e == Result_Ok)
			result = BUTTON_OK;
		else if(e == Result_Cancel)
			result = BUTTON_CANCEL;
	}
}

//=================================================================================================
GetNumberDialog* GetNumberDialog::Show(Control* parent, DialogEvent event, cstring text, int minValue, int maxValue, int* value)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "GetNumberDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = ORDER_NORMAL;
		info.type = DIALOG_CUSTOM;

		self = new GetNumberDialog(info);
		self->size = Int2(300, 200);
		self->bts.resize(2);

		Button& bt1 = self->bts[0],
			&bt2 = self->bts[1];

		bt1.text = gui->txCancel;
		bt1.id = Result_Cancel;
		bt1.size = Int2(100, 40);
		bt1.pos = Int2(300 - 100 - 16, 200 - 40 - 16);
		bt1.parent = self;

		bt2.text = gui->txOk;
		bt2.id = Result_Ok;
		bt2.size = Int2(100, 40);
		bt2.pos = Int2(16, 200 - 40 - 16);
		bt2.parent = self;

		self->textBox.size = Int2(200, 35);
		self->textBox.pos = Int2(50, 60);
		self->textBox.SetNumeric(true);
		self->textBox.limit = 10;

		Scrollbar& scroll = self->scrollbar;
		scroll.pos = Int2(32, 100);
		scroll.size = Int2(300 - 64, 16);
		scroll.total = 100;
		scroll.part = 5;

		gui->RegisterControl(self);
	}

	self->result = -1;
	self->parent = parent;
	self->order = static_cast<DialogBox*>(parent)->order;
	self->event = event;
	self->text = text;
	self->minValue = minValue;
	self->maxValue = maxValue;
	self->value = value;
	self->pos = self->globalPos = (gui->wndSize - self->size) / 2;
	self->bts[0].globalPos = self->bts[0].pos + self->globalPos;
	self->bts[1].globalPos = self->bts[1].pos + self->globalPos;
	self->textBox.globalPos = self->textBox.pos + self->globalPos;
	self->textBox.num_min = minValue;
	self->textBox.num_max = maxValue;
	self->scrollbar.globalPos = self->scrollbar.pos + self->globalPos;
	self->scrollbar.offset = 0;
	self->scrollbar.manual_change = true;
	self->textBox.SetText(Format("%d", *value));

	gui->ShowDialog(self);

	return self;
}
