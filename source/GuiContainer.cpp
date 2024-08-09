#include "Pch.h"
#include "GuiContainer.h"

#include "Input.h"

//=================================================================================================
GuiContainer::GuiContainer() : focus(false), focusCtrl(nullptr), giveFocus(nullptr)
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
	// przeka¿ focus kontrolce
	if(focus && giveFocus)
		CheckGiveFocus();

	// aktualizuj
	// jeœli nic nie jest aktywne to aktywuj pierwsz¹ kontrolkê
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(IsSet(it->second, F_MOUSE_FOCUS))
			it->first->mouseFocus = focus;
		if(focus)
		{
			if(!focusCtrl && IsSet(it->second, F_FOCUS))
			{
				focusCtrl = it->first;
				focusCtrl->GainFocus();
			}
			if(IsSet(it->second, F_CLICK_TO_FOCUS))
			{
				if(it->first->IsInside(gui->cursorPos) && (input->Pressed(Key::LeftButton) || input->Pressed(Key::RightButton)))
				{
					if(!it->first->focus)
					{
						if(focusCtrl)
							focusCtrl->LostFocus();
						focusCtrl = it->first;
						focusCtrl->GainFocus();
					}
				}
			}
		}
		it->first->Update(dt);
	}

	// prze³¹czanie
	if(focus && input->PressedRelease(Key::Tab))
	{
		// znajdŸ aktualny element
		Iter begin = items.begin(), end = items.end(), start;
		for(start = begin; start != end; ++start)
		{
			if(start->first == focusCtrl)
				break;
		}
		if(start == end)
			return;
		Iter newItem = end;
		if(input->Down(Key::Shift))
		{
			// znajdŸ poprzedni
			// od start do begin
			if(start != begin)
			{
				for(Iter it = start - 1; it != begin; --it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						newItem = it;
						break;
					}
				}
			}
			// od end do start
			if(newItem == end)
			{
				for(Iter it = end - 1; it != start; --it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						newItem = it;
						break;
					}
				}
			}
		}
		else
		{
			// znajdŸ nastêpny
			// od start do end
			for(Iter it = start + 1; it != end; ++it)
			{
				if(IsSet(it->second, F_FOCUS))
				{
					newItem = it;
					break;
				}
			}
			// od begin do start
			if(newItem == end)
			{
				for(Iter it = begin; it != start; ++it)
				{
					if(IsSet(it->second, F_FOCUS))
					{
						newItem = it;
						break;
					}
				}
			}
		}

		// zmiana focus
		if(newItem != end)
		{
			if(focusCtrl)
				focusCtrl->LostFocus();
			focusCtrl = newItem->first;
			focusCtrl->GainFocus();
		}
	}
}

//=================================================================================================
void GuiContainer::GainFocus()
{
	if(focus)
		return;

	focus = true;
	// aktywuj coœ
	if(giveFocus)
		CheckGiveFocus();
	else if(!focusCtrl)
	{
		for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		{
			if(IsSet(it->second, F_FOCUS))
			{
				focusCtrl = it->first;
				focusCtrl->GainFocus();
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
	if(focusCtrl)
	{
		focusCtrl->LostFocus();
		focusCtrl = nullptr;
	}
}

//=================================================================================================
void GuiContainer::Move(const Int2& globalPos)
{
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
		it->first->globalPos = globalPos + it->first->pos;
}

//=================================================================================================
void GuiContainer::CheckGiveFocus()
{
	bool ok = false;
	for(Iter it = items.begin(), end = items.end(); it != end; ++it)
	{
		if(it->first == giveFocus)
		{
			if(focusCtrl != it->first)
			{
				if(focusCtrl)
					focusCtrl->LostFocus();
				focusCtrl = giveFocus;
				focusCtrl->GainFocus();
			}
			ok = true;
			break;
		}
	}
	giveFocus = nullptr;
	assert(ok);
}
