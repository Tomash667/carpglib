#pragma once

#include "Window.h"

class GuiDialog : public Window
{
	friend class Overlay;
public:
	GuiDialog();
	void Update(float dt) override;
	void Show();
	void Close();
	void SetHandler(delegate<void()> handler) { this->handler = handler; }

protected:
	delegate<void()> handler;
	bool allowClose;
};
