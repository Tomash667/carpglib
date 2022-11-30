#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Button.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Slider : public Control
	{
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class Slider : public Control, public LayoutControl<layout::Slider>
{
public:
	Slider();

	void Draw() override;
	void Update(float dt) override;
	void Event(GuiEvent e) override;

	void SetHold(bool hold);

	int minv, maxv, val, id;
	string text;
	Button bt[2];
	float holdVal;

private:
	float holdTmp;
	int holdState;
	bool hold, minstep;
};
