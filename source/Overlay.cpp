#include "Pch.h"
#include "Overlay.h"

#include "DialogBox.h"
#include "GuiDialog.h"
#include "Input.h"
#include "MenuStrip.h"

Overlay::Overlay() : Container(true), focused(nullptr), toAdd(nullptr), drawCursor(true)
{
}

Overlay::~Overlay()
{
	// prevent deleting twice
	for(MenuStrip* menu : menus)
		RemoveElement(ctrls, static_cast<Control*>(menu));
}

bool Overlay::NeedCursor() const
{
	if(drawCursor || HaveDialog())
		return true;

	return Container::NeedCursor();
}

void Overlay::Draw()
{
	Container::Draw();

	for(GuiDialog* dialog : dialogs)
	{
		gui->DrawArea(Box2d::Create(Int2::Zero, wndSize), layout->background);
		dialog->Draw();
	}

	for(MenuStrip* menu : menus)
		menu->Draw();
}

void Overlay::Update(float dt)
{
	mouseFocus = true;
	clicked = nullptr;

	// close menu if old dialog window is open
	if(gui->HaveDialog())
	{
		mouseFocus = false;
		CloseMenus();
	}

	for(MenuStrip* menu : menus) // kaplica jak coœ usunie
		UpdateControl(menu, dt);

	// update dialogs
	for(auto it = dialogs.rbegin(), end = dialogs.rend(); it != end; ++it)
		UpdateControl(*it, dt);
	if(!dialogs.empty())
		mouseFocus = false;
	for(GuiDialog* dialog : dialogsToClose)
	{
		dialog->Event(GuiEvent_Close);
		RemoveElement(dialogs, dialog);
	}
	dialogsToClose.clear();

	// update controls
	Container::Update(dt);

	// close menu if clicked outside
	if(clicked)
	{
		MenuStrip* leftover = nullptr;
		for(MenuStrip* menu : menus)
		{
			if(menu != clicked)
				menu->OnClose();
			else
				leftover = menu;
		}
		menus.clear();
		if(leftover)
			menus.push_back(leftover);
	}

	for(MenuStrip* menu : menusToClose)
		RemoveElement(menus, menu);
	menusToClose.clear();

	if(toAdd)
	{
		menus.push_back(toAdd);
		if(focused)
		{
			focused->focus = false;
			focused->Event(GuiEvent_LostFocus);
			if(focused->IsOnCharHandler())
				gui->RemoveOnCharHandler(dynamic_cast<OnCharHandler*>(focused));
		}
		toAdd->focus = true;
		focused = toAdd;
		if(toAdd->IsOnCharHandler())
			gui->AddOnCharHandler(dynamic_cast<OnCharHandler*>(toAdd));
		toAdd = nullptr;
	}
}

void Overlay::ShowDialog(GuiDialog* dialog)
{
	assert(dialog);
	CloseMenus();
	dialogs.push_back(dialog);
	dialog->Initialize();
	dialog->SetPosition((wndSize - dialog->GetSize()) / 2);
}

void Overlay::CloseDialog(GuiDialog* dialog)
{
	assert(dialog);
	dialogsToClose.push_back(dialog);
}

void Overlay::ShowMenu(MenuStrip* menu, const Int2& pos)
{
	assert(menu);
	CloseMenus();
	toAdd = menu;
	menu->ShowAt(pos);
}

void Overlay::CloseMenu(MenuStrip* menu)
{
	assert(menu);
	menu->OnClose();
	menusToClose.push_back(menu);
}

void Overlay::CheckFocus(Control* ctrl, bool pressed)
{
	assert(ctrl);
	if(!ctrl->mouseFocus)
		return;

	ctrl->mouseFocus = false;

	if(input->PressedRelease(Key::LeftButton)
		|| input->PressedRelease(Key::RightButton)
		|| input->PressedRelease(Key::MiddleButton)
		|| pressed)
	{
		assert(!clicked);
		clicked = ctrl;
		SetFocus(ctrl);
	}
}

void Overlay::SetFocus(Control* ctrl)
{
	assert(ctrl);
	if(focused)
	{
		if(focused == ctrl)
			return;
		focused->focus = false;
		focused->Event(GuiEvent_LostFocus);
		if(focused->IsOnCharHandler())
			gui->RemoveOnCharHandler(dynamic_cast<OnCharHandler*>(focused));
	}
	ctrl->focus = true;
	ctrl->Event(GuiEvent_GainFocus);
	focused = ctrl;
	if(ctrl->IsOnCharHandler())
		gui->AddOnCharHandler(dynamic_cast<OnCharHandler*>(ctrl));
}

void Overlay::ClearFocus()
{
	if(focused)
	{
		focused->focus = false;
		focused->Event(GuiEvent_LostFocus);
		if(focused->IsOnCharHandler())
			gui->RemoveOnCharHandler(dynamic_cast<OnCharHandler*>(focused));
		focused = nullptr;
	}
}

bool Overlay::IsOpen(MenuStrip* menu)
{
	assert(menu);

	for(MenuStrip* m : menus)
	{
		if(m == menu)
			return true;
	}

	return false;
}

void Overlay::CloseMenus()
{
	for(MenuStrip* menu : menus)
		menu->OnClose();
	menus.clear();
	if(toAdd)
	{
		toAdd->OnClose();
		toAdd = nullptr;
	}
}

bool Overlay::HaveDialog() const
{
	return !dialogs.empty() || gui->HaveDialog();
}
