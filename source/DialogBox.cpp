#include "Pch.h"
#include "DialogBox.h"

#include "Input.h"

//=================================================================================================
DialogBox::DialogBox(const DialogInfo& info) : name(info.name), text(info.text), type(info.type), event(info.event), order(info.order), pause(info.pause),
needDelete(false), result(-1)
{
	parent = info.parent;
	focusable = true;
	visible = false;
}

//=================================================================================================
void DialogBox::Draw()
{
	DrawPanel();

	for(Button& button : bts)
		button.Draw();

	Rect r = { pos.x + 12, pos.y + 12, pos.x + size.x - 12, pos.y + size.y - 12 };
	gui->DrawText(layout->font, text, DTF_CENTER, Color::Black, r);
}

//=================================================================================================
void DialogBox::DrawPanel(bool background)
{
	if(background)
		gui->DrawArea(Box2d::Create(Int2::Zero, gui->wndSize), layout->background);
	gui->DrawArea(Box2d::Create(pos, size), layout->box);
}

//=================================================================================================
void DialogBox::Update(float dt)
{
	result = -1;

	for(Button& button : bts)
	{
		button.mouseFocus = focus;
		button.Update(dt);
	}

	if(input->Focus() && focus && result == -1)
	{
		if(bts[0].state != Button::DISABLED)
		{
			if(input->PressedRelease(Key::Escape))
				result = (type == DIALOG_OK ? BUTTON_OK : BUTTON_NO);
			else if(input->PressedRelease(Key::Enter) || input->PressedRelease(Key::Spacebar))
				result = (type == DIALOG_OK ? BUTTON_OK : BUTTON_YES);
		}
	}

	if(result != -1)
	{
		if(event)
			event(result);
		gui->CloseDialog(this);
	}
}

//=================================================================================================
void DialogBox::Event(GuiEvent e)
{
	if(e >= GuiEvent_Custom)
		result = e - GuiEvent_Custom;
	else if(e == GuiEvent_Show || e == GuiEvent_Resize)
	{
		pos = globalPos = (gui->wndSize - size) / 2;
		for(uint i = 0; i < bts.size(); ++i)
			bts[i].globalPos = bts[i].pos + pos;
	}
}

//=================================================================================================
DialogOrder DialogBox::GetOrder(Control* control)
{
	DialogBox* dialogBox = dynamic_cast<DialogBox*>(control);
	return dialogBox ? dialogBox->order : DialogOrder::Normal;
}

//=================================================================================================
DialogWithCheckbox::DialogWithCheckbox(const DialogInfo& info) : DialogBox(info)
{
}

//=================================================================================================
void DialogWithCheckbox::Draw()
{
	DialogBox::Draw();

	checkbox.Draw();
}

//=================================================================================================
void DialogWithCheckbox::Update(float dt)
{
	if(result == -1)
	{
		checkbox.mouseFocus = focus;
		checkbox.Update(dt);
	}

	DialogBox::Update(dt);
}

//=================================================================================================
void DialogWithCheckbox::Event(GuiEvent e)
{
	if(e >= GuiEvent_Custom)
	{
		if(e == GuiEvent_Custom + BUTTON_CHECKED)
			event(checkbox.checked ? BUTTON_CHECKED : BUTTON_UNCHECKED);
		else
			result = e - GuiEvent_Custom;
	}
	else if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		DialogBox::Event(e);
		checkbox.globalPos = checkbox.pos + pos;
	}
}

//=================================================================================================
DialogWithImage::DialogWithImage(const DialogInfo& info) : DialogBox(info), img(info.img)
{
	assert(img);
	imgSize = img->GetSize();
}

//=================================================================================================
void DialogWithImage::Draw()
{
	DrawPanel();

	for(uint i = 0; i < bts.size(); ++i)
	{
		bts[i].globalPos = bts[i].pos + pos;
		bts[i].Draw();
	}

	Rect r = textRect + pos;
	gui->DrawText(layout->font, text, DTF_CENTER, Color::Black, r);

	gui->DrawSprite(img, imgPos + pos);
}

//=================================================================================================
void DialogWithImage::Setup(const Int2& textSize)
{
	imgPos = Int2(12, (max(textSize.y, imgSize.y) - imgSize.y) / 2);
	textRect = Rect::Create(Int2(imgPos.x + imgSize.x + 8, 12), textSize);
}
