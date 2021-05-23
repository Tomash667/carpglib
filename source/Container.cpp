#include "Pch.h"
#include "Container.h"

//=================================================================================================
Container::~Container()
{
	if(new_mode)
		DeleteElements(ctrls);
}

//=================================================================================================
void Container::Add(Control* ctrl)
{
	assert(ctrl && ctrl != this);
	ctrl->parent = this;
	ctrl->global_pos = global_pos + ctrl->pos;
	if(disabled)
		ctrl->SetDisabled(true);
	ctrls.push_back(ctrl);
	inside_loop = false;

	if(IsInitialized())
		ctrl->Initialize();

	if(ctrl->IsDocked())
		Dock(ctrl);
}

//=================================================================================================
bool Container::AnythingVisible() const
{
	for(vector<Control*>::const_iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
	{
		if((*it)->visible)
			return true;
	}
	return false;
}

//=================================================================================================
void Container::Draw(ControlDrawData* cdd)
{
	for(Control* c : ctrls)
	{
		if(!c->visible)
			continue;

		if(auto_focus)
			c->focus = focus;
		c->Draw(cdd);
	}
}

//=================================================================================================
void Container::Update(float dt)
{
	inside_loop = true;

	if(new_mode)
	{
		for(vector<Control*>::reverse_iterator it = ctrls.rbegin(), end = ctrls.rend(); it != end; ++it)
		{
			Control& c = **it;
			if(!c.visible)
				continue;
			UpdateControl(&c, dt);
			if(!inside_loop)
				return;
		}
	}
	else if(dont_focus)
	{
		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
			{
				(*it)->LostFocus();
				(*it)->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}
	else if(focus_top)
	{
		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
			{
				if(focus && (*it)->focusable && it == end - 1)
					(*it)->GainFocus();
				else
					(*it)->LostFocus();
				(*it)->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}
	else
	{
		Control* top = nullptr;
		if(focus)
		{
			for(Control* c : ctrls)
			{
				if(c->visible && c->focusable)
				{
					top = c;
					break;
				}
			}
		}

		for(Control* c : ctrls)
		{
			__assume(c != nullptr);
			if(c->visible)
			{
				if(c == top)
					c->GainFocus();
				else
					c->LostFocus();
				c->Update(dt);
				if(!inside_loop)
					return;
			}
		}
	}

	inside_loop = false;
}

//=================================================================================================
void Container::Event(GuiEvent e)
{
	if(new_mode)
	{
		switch(e)
		{
		case GuiEvent_Initialize:
			for(Control* c : ctrls)
				c->Initialize();
			break;
		case GuiEvent_WindowResize:
		case GuiEvent_Show:
		case GuiEvent_Hide:
			for(Control* c : ctrls)
				c->Event(e);
			break;
		case GuiEvent_Moved:
			for(Control* c : ctrls)
			{
				c->global_pos = c->pos + global_pos;
				c->Event(GuiEvent_Moved);
			}
			break;
		default:
			if(e >= GuiEvent_Custom)
				parent->Event(e);
			break;
		}
	}
	else if(e == GuiEvent_WindowResize)
	{
		for(Control* ctrl : ctrls)
			ctrl->Event(GuiEvent_WindowResize);
	}
}

//=================================================================================================
bool Container::NeedCursor() const
{
	for(const Control* c : ctrls)
	{
		if(c->visible && c->NeedCursor())
			return true;
	}
	return false;
}

//=================================================================================================
void Container::SetDisabled(bool new_disabled)
{
	if(new_disabled == disabled)
		return;

	for(Control* c : ctrls)
		c->SetDisabled(new_disabled);
	disabled = new_disabled;
}

//=================================================================================================
void Container::Remove(Control* ctrl)
{
	assert(ctrl);

	if(ctrls.back() == ctrl)
	{
		ctrls.pop_back();
		if(!ctrls.empty())
		{
			ctrls.back()->focus = true;
			ctrls.back()->Event(GuiEvent_GainFocus);
		}
	}
	else
		RemoveElementOrder(ctrls, ctrl);

	inside_loop = false;
}

//=================================================================================================
Control* Container::HitTest()
{
	for(Control* ctrl : ctrls)
	{
		if(ctrl->visible && ctrl->IsInside(gui->cursor_pos))
			return ctrl;
	}
	return nullptr;
}
