#include "Pch.h"
#include "Container.h"

//=================================================================================================
Container::~Container()
{
	if(isNew && !manual)
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
	insideLoop = false;

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
void Container::Draw()
{
	for(Control* c : ctrls)
	{
		if(!c->visible)
			continue;

		if(autoFocus)
			c->focus = focus;
		c->Draw();
	}
}

//=================================================================================================
void Container::Update(float dt)
{
	insideLoop = true;

	if(isNew)
	{
		for(vector<Control*>::reverse_iterator it = ctrls.rbegin(), end = ctrls.rend(); it != end; ++it)
		{
			Control& c = **it;
			if(!c.visible)
				continue;
			UpdateControl(&c, dt);
			if(!insideLoop)
				return;
		}
	}
	else if(dontFocus)
	{
		for(vector<Control*>::iterator it = ctrls.begin(), end = ctrls.end(); it != end; ++it)
		{
			if((*it)->visible)
			{
				(*it)->LostFocus();
				(*it)->Update(dt);
				if(!insideLoop)
					return;
			}
		}
	}
	else if(focusTop)
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
				if(!insideLoop)
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
				if(!insideLoop)
					return;
			}
		}
	}

	insideLoop = false;
}

//=================================================================================================
void Container::Event(GuiEvent e)
{
	if(isNew)
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
				c->globalPos = c->pos + globalPos;
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
void Container::SetDisabled(bool disabled)
{
	if(this->disabled == disabled)
		return;

	for(Control* c : ctrls)
		c->SetDisabled(disabled);
	this->disabled = disabled;
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

	insideLoop = false;
}

//=================================================================================================
Control* Container::HitTest()
{
	for(Control* ctrl : ctrls)
	{
		if(ctrl->visible && ctrl->IsInside(gui->cursorPos))
			return ctrl;
	}
	return nullptr;
}
