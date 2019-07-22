#include "EnginePch.h"
#include "EngineCore.h"
#include "GuiDialog.h"
#include "Overlay.h"

void GuiDialog::Close()
{
	gui->GetOverlay()->CloseDialog(this);
}
