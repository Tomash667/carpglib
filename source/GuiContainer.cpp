#include "Pch.h"
#include "GuiContainer.h"

#include "Input.h"

//=================================================================================================
GuiContainer::GuiContainer() : focus(false), focus_ctrl(nullptr), give_focus(nullptr)
{
}

//=================================================================================================
void GuiContainer::Draw()
{
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		it->first->Draw();
}

//=================================================================================================
void GuiContainer::Update(float dt)
{
	// przeka� focus kontrolce
	if(focus && give_focus)
		CheckGiveFocus();

	// aktualizuj
	// je�li nic nie jest aktywne to aktywuj pierwsz� kontrolk�
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(IsSet(it->second, F_MOUSE_FOCUS))
			it->first->mouse_focus = focus;
		if(focus)
		{
			if(!focus_ctrl && IsSet(it->second, F_FOCUS))
			{
				focus_ctrl = it->first;
				focus_ctrl->GainFocus();
			}
			if(IsSet(it->second, F_CLICK_TO_FOCUS))
			{
				if(it->first->IsInside(gui->cursorPos) && (input->Pressed(Key::LeftButton) || input->Pressed(Key::RightButton)))
				{
					if(!it->first->focus)
					{
						if(focus_ctrl)
							focus_ctrl->LostFocus();
						focus_ctrl = it->first;
						focus_ctrl->GainFocus();
					}
				}
			}
		}
		it->first->Update(dt);
	}

	// prze��czanie
	if(focus && input->Focus() && input->PressedRelease(Key::Tab))
	{
		// znajd� aktualny element
		Iter begin = items.begin(), end = items.end(), start;
		for(start = begin; start != end; ++start)
		{
			if(start->first == focus_ctrl)
				break;
		}
		if(start == end)
			return;
		Iter new_item = end;
		if(input->Down(Key::Shift))
		{
			// znajd� poprzedni
			// od start do begin
			if(start != begin)
			{
				for(Iter it = start - 1; it != begin; --it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
			// od end do start
			if(new_item == end)
			{
				for(Iter it = end - 1; it != start; --it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
		}
		else
		{
			// znajd� nast�pny
			// od start do end
			for(Iter it = start + 1; it != end; ++it)
			{
				if(IsSet(it->second, F_FOCUS))
				{
					new_item = it;
					break;
				}
			}
			// od begin do start
			if(new_item == end)
			{
				for(Iter it = begin; it != start; ++it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						new_item = it;
						break;
					}
				}
			}
		}

		// zmiana focus
		if(new_item != end)
		{
			if(focus_ctrl)
				focus_ctrl->LostFocus();
			focus_ctrl = new_item->first;
			focus_ctrl->GainFocus();
		}
	}
}

//=================================================================================================
void GuiContainer::GainFocus()
{
	if(focus)
		return;

	focus = true;
	// aktywuj co�
	if(give_focus)
		CheckGiveFocus();
	else if(!focus_ctrl)
	{
		for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(IsSet(it->second, F_FOCUS))
			{
				focus_ctrl = it->first;
				focus_ctrl->GainFocus();
				break;
			}
		}
	}
}

//=================================================================================================
void GuiContainer::LostFocus()
{
	if(!focus)
		return;

	focus = false;
	if(focus_ctrl)
	{
		focus_ctrl->LostFocus();
		focus_ctrl = nullptr;
	}
}

//=================================================================================================
void GuiContainer::Move(const Int2& global_pos)
{
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		it->first->global_pos = global_pos + it->first->pos;
}

//=================================================================================================
void GuiContainer::CheckGiveFocus()
{
	bool ok = false;
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->first == give_focus)
		{
			if(focus_ctrl != it->first)
			{
				if(focus_ctrl)
					focus_ctrl->LostFocus();
				focus_ctrl = give_focus;
				focus_ctrl->GainFocus();
			}
			ok = true;
			break;
		}
	}
	give_focus = nullptr;
	assert(ok);
}
