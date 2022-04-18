#include "Pch.h"
#include "GuiDialog.h"

#include "Input.h"
#include "Overlay.h"

void GuiDialog::Update(float dt)
{
	if(input->Pressed(Key::Escape))
		Close();
	Window::Update(dt);
}

void GuiDialog::Close()
{
	gui->GetOverlay()->CloseDialog(this);
}
