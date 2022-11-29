#include "Pch.h"
#include "Control.h"

#include "Overlay.h"

Gui* Control::gui;
Input* Control::input;

void Control::Dock(Control* c)
{
	assert(c);
	if(c->globalPos != globalPos)
	{
		c->globalPos = globalPos;
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

void Control::GainFocus()
{
	if(!focus)
	{
		focus = true;
		Event(GuiEvent_GainFocus);
	}
}

void Control::LostFocus()
{
	if(focus)
	{
		focus = false;
		Event(GuiEvent_LostFocus);
	}
}

void Control::Show(bool visible)
{
	if(this->visible != visible)
	{
		this->visible = visible;
		Event(visible ? GuiEvent_Show : GuiEvent_Hide);
	}
}

void Control::SetSize(const Int2& newSize)
{
	if(size == newSize || IsDocked())
		return;
	size = newSize;
	if(initialized)
		Event(GuiEvent_Resize);
}

void Control::SetPosition(const Int2& newPos)
{
	if(pos == newPos || IsDocked())
		return;
	globalPos -= pos;
	pos = newPos;
	globalPos += pos;
	if(initialized)
		Event(GuiEvent_Moved);
}

void Control::SetDocked(bool newDocked)
{
	if(IsDocked() == newDocked)
		return;
	SetBitValue(flags, F_DOCKED, newDocked);
	if(newDocked)
	{
		if(parent)
			parent->Dock(this);
	}
	else
	{
		Int2 new_global_pos = parent->globalPos + pos;
		if(new_global_pos != globalPos)
		{
			globalPos = new_global_pos;
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
	if(mouseFocus && ctrl->IsInside(gui->cursorPos))
	{
		ctrl->mouseFocus = true;
		ctrl->Update(dt);
		if(!ctrl->mouseFocus)
			mouseFocus = false;
		else
			ctrl->TakeFocus();
	}
	else
	{
		ctrl->mouseFocus = false;
		ctrl->Update(dt);
	}
}
