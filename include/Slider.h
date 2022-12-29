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
	void SetHold(float holdVal);

	int width, minv, maxv, val, id;
	string text;

private:
	Button bt[2];
	float holdVal, holdTmp;
	int holdState;
	bool minstep;
};
