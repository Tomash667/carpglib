#include "EnginePch.h"
#include "EngineCore.h"
#include "DialogBox.h"
#include "Input.h"

//=================================================================================================
DialogBox::DialogBox(const DialogInfo& info) : name(info.name), text(info.text), type(info.type), event(info.event), order(info.order), pause(info.pause),
need_delete(false), result(-1)
{
	parent = info.parent;
	focusable = true;
	visible = false;
}

//=================================================================================================
void DialogBox::Draw(ControlDrawData*)
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
		gui->DrawArea(Box2d::Create(Int2::Zero, gui->wnd_size), layout->background);
	gui->DrawArea(Box2d::Create(pos, size), layout->box);
}

//=================================================================================================
void DialogBox::Update(float dt)
{
	result = -1;

	for(Button& button : bts)
	{
		button.mouse_focus = focus;
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
		pos = global_pos = (gui->wnd_size - size) / 2;
		for(uint i = 0; i < bts.size(); ++i)
			bts[i].global_pos = bts[i].pos + pos;
	}
}

//=================================================================================================
DialogWithCheckbox::DialogWithCheckbox(const DialogInfo& info) : DialogBox(info)
{
}

//=================================================================================================
void DialogWithCheckbox::Draw(ControlDrawData*)
{
	DialogBox::Draw();

	checkbox.Draw();
}

//=================================================================================================
void DialogWithCheckbox::Update(float dt)
{
	if(result == -1)
	{
		checkbox.mouse_focus = focus;
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
		checkbox.global_pos = checkbox.pos + pos;
	}
}

//=================================================================================================
DialogWithImage::DialogWithImage(const DialogInfo& info) : DialogBox(info), img(info.img)
{
	assert(img);
	img_size = img->GetSize();
}

//=================================================================================================
void DialogWithImage::Draw(ControlDrawData*)
{
	DrawPanel();

	for(uint i = 0; i < bts.size(); ++i)
	{
		bts[i].global_pos = bts[i].pos + pos;
		bts[i].Draw();
	}

	Rect r = text_rect + pos;
	gui->DrawText(layout->font, text, DTF_CENTER, Color::Black, r);

	gui->DrawSprite(img, img_pos + pos);
}

//=================================================================================================
void DialogWithImage::Setup(const Int2& text_size)
{
	img_pos = Int2(12, (max(text_size.y, img_size.y) - img_size.y) / 2);
	text_rect = Rect::Create(Int2(img_pos.x + img_size.x + 8, 12), text_size);
}
