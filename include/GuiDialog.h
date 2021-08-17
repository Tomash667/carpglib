#pragma once

#include "Window.h"

class GuiDialog : public Window
{
public:
	GuiDialog();
	void Update(float dt) override;
	void Close();

protected:
	bool allowClose;
};
