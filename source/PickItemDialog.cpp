#include "Pch.h"
#include "PickItemDialog.h"

#include "Input.h"

//-----------------------------------------------------------------------------
PickItemDialog* PickItemDialog::self;

//=================================================================================================
void PickItemDialogParams::AddItem(cstring itemText, int id, int group, bool disabled)
{
	FlowItem* item = FlowItem::Pool.Get();
	item->type = FlowItem::Item;
	item->group = group;
	item->id = id;
	item->text = itemText;
	item->state = (!disabled ? Button::NONE : Button::DISABLED);
	items.push_back(item);
}

//=================================================================================================
void PickItemDialogParams::AddSeparator(cstring itemText)
{
	FlowItem* item = FlowItem::Pool.Get();
	item->type = FlowItem::Section;
	item->text = itemText;
	item->state = Button::NONE;
	items.push_back(item);
}

//=================================================================================================
PickItemDialog::PickItemDialog(const DialogInfo&  info) : DialogBox(info)
{
	DialogBox::layout = layout;

	flow.allow_select = true;
	flow.on_select = VoidF(this, &PickItemDialog::OnSelect);

	btClose.custom = &layout->close;
	btClose.id = Cancel;
	btClose.size = Int2(32, 32);
	btClose.parent = this;
}

//=================================================================================================
PickItemDialog* PickItemDialog::Show(PickItemDialogParams& params)
{
	if(!self)
	{
		DialogInfo info;
		info.event = nullptr;
		info.name = "PickItemDialog";
		info.parent = nullptr;
		info.pause = false;
		info.order = DialogOrder::Normal;
		info.type = DialogType::Custom;

		self = new PickItemDialog(info);

		gui->RegisterControl(self);
	}

	self->Create(params);

	gui->ShowDialog(self);

	return self;
}

//=================================================================================================
void PickItemDialog::Create(PickItemDialogParams& params)
{
	result = -1;
	parent = params.parent;
	order = GetOrder(params.parent);
	event = params.event;
	getTooltip = params.getTooltip;
	if(params.getTooltip)
		tooltip.Init(params.getTooltip);
	multiple = params.multiple;
	size.x = params.sizeMin.x;
	text = std::move(params.text);
	flow.pos = Int2(16, 64);
	flow.size = Int2(size.x - 32, 10000);
	flow.SetItems(params.items);
	int flowHeight = flow.GetHeight();
	flowHeight += 64;
	if(flowHeight < params.sizeMin.y)
		flowHeight = params.sizeMin.y;
	else if(flowHeight > params.sizeMax.y)
		flowHeight = params.sizeMax.y;
	size.y = flowHeight;
	pos = global_pos = (gui->wndSize - size) / 2;
	flow.UpdateSize(Int2(16, 64), Int2(size.x - 32, size.y - 80), true);
	btClose.pos = Int2(size.x - 48, 16);
	btClose.global_pos = global_pos + btClose.pos;
	selected.clear();
}

//=================================================================================================
void PickItemDialog::Draw(ControlDrawData*)
{
	DrawPanel();

	btClose.Draw();

	Rect r = { global_pos.x + 16, global_pos.y + 16, global_pos.x + size.x - 56, global_pos.y + size.y };
	gui->DrawText(layout->font, text, DTF_CENTER, Color::Black, r);

	flow.Draw();
	if(getTooltip)
		tooltip.Draw();
}

//=================================================================================================
void PickItemDialog::Update(float dt)
{
	btClose.mouse_focus = focus;
	btClose.Update(dt);

	flow.mouse_focus = focus;
	flow.Update(dt);

	if(getTooltip)
	{
		int group = -1, id = -1;
		flow.GetSelected(group, id);
		tooltip.UpdateTooltip(dt, group, id);
	}

	if(result == -1)
	{
		if(input->PressedRelease(Key::Escape))
		{
			result = BUTTON_CANCEL;
			gui->CloseDialog(this);
			if(event)
				event(result);
		}
	}
}

//=================================================================================================
void PickItemDialog::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		// recenter
		pos = global_pos = (gui->wndSize - size) / 2;
		flow.UpdatePos(pos);
		btClose.global_pos = global_pos + btClose.pos;
	}
	else if(e == Cancel)
	{
		result = BUTTON_CANCEL;
		gui->CloseDialog(this);
		if(event)
			event(result);
	}
}

//=================================================================================================
void PickItemDialog::OnSelect()
{
	if(multiple == 0)
	{
		// single selection, return result
		result = BUTTON_OK;
		selected.push_back(flow.selected);
		gui->CloseDialog(this);
		if(event)
			event(result);
	}
	else
	{
		// multiple selection
		FlowItem* item = flow.selected;
		if(item->state == Button::DOWN)
		{
			// remove selection
			item->state = Button::NONE;
			RemoveElement(selected, item);
		}
		else
		{
			// add selection
			item->state = Button::DOWN;
			selected.push_back(item);
			if(selected.size() == (uint)multiple)
			{
				// selected requested count of items, return result
				result = BUTTON_OK;
				gui->CloseDialog(this);
				if(event)
					event(result);
			}
		}
	}
}
