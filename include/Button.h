#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
struct CustomButton
{
	AreaLayout tex[4];
};

//-----------------------------------------------------------------------------
namespace layout
{
	struct Button : public Control
	{
		AreaLayout tex[4];
		Font* font;
		int padding;
	};
}

//-----------------------------------------------------------------------------
class Button : public Control, public LayoutControl<layout::Button>
{
public:
	enum State
	{
		NONE,
		HOVER,
		DOWN,
		DISABLED
	};

	Button();

	void Draw() override;
	DialogEvent GetHandler() const { return handler; }
	void SetHandler(DialogEvent handler) { this->handler = handler; }
	void Update(float dt) override;

	string text;
	State state;
	int id;
	Texture* img;
	Int2 forceImgSize;
	CustomButton* custom;
	bool hold;

private:
	DialogEvent handler;
};
