#include "EnginePch.h"
#include "EngineCore.h"
#include "Control.h"
#include "Overlay.h"
#include "DirectX.h"

Gui* Control::gui;
Input* Control::input;

void Control::Dock(Control* c)
{
	assert(c);
	if(c->global_pos != global_pos)
	{
		c->global_pos = global_pos;
		if(c->IsInitialized())
			c->Event(GuiEvent_Moved);
	}
	if(c->size != size)
	{
		c->size = size;
		if(c->IsInitialized())
			c->Event(GuiEvent_Resize);
	}
}

void Control::Initialize()
{
	if(initialized)
		return;
	initialized = true;
	Event(GuiEvent_Initialize);
}

void Control::SetSize(const Int2& new_size)
{
	if(size == new_size || IsDocked())
		return;
	size = new_size;
	if(initialized)
		Event(GuiEvent_Resize);
}

void Control::SetPosition(const Int2& new_pos)
{
	if(pos == new_pos || IsDocked())
		return;
	global_pos -= pos;
	pos = new_pos;
	global_pos += pos;
	if(initialized)
		Event(GuiEvent_Moved);
}

void Control::SetDocked(bool new_docked)
{
	if(IsDocked() == new_docked)
		return;
	SET_BIT_VALUE(flags, F_DOCKED, new_docked);
	if(new_docked)
	{
		if(parent)
			parent->Dock(this);
	}
	else
	{
		Int2 new_global_pos = parent->global_pos + pos;
		if(new_global_pos != global_pos)
		{
			global_pos = new_global_pos;
			Event(GuiEvent_Moved);
		}
	}
}

void Control::TakeFocus(bool pressed)
{
	gui->GetOverlay()->CheckFocus(this, pressed);
}

void Control::SetFocus()
{
	gui->GetOverlay()->SetFocus(this);
}

void Control::UpdateControl(Control* ctrl, float dt)
{
	if(mouse_focus && ctrl->IsInside(gui->cursor_pos))
	{
		ctrl->mouse_focus = true;
		ctrl->Update(dt);
		if(!ctrl->mouse_focus)
			mouse_focus = false;
		else
			ctrl->TakeFocus();
	}
	else
	{
		ctrl->mouse_focus = false;
		ctrl->Update(dt);
	}
}
