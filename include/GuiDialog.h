#pragma once

#include "Window.h"

class GuiDialog : public Window
{
public:
	void Update(float dt) override;
	void Close();
};
