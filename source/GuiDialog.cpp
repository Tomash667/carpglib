#include "Pch.h"
#include "GuiDialog.h"

#include "Input.h"
#include "Overlay.h"

GuiDialog::GuiDialog() : allowClose(false)
{
}

void GuiDialog::Update(float dt)
{
	if(allowClose && focus && input->PressedRelease(Key::Escape))
	{
		Close();
		return;
	}

	Window::Update(dt);
}

void GuiDialog::Close()
{
	gui->GetOverlay()->CloseDialog(this);
}
