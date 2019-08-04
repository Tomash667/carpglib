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
	};
}

//-----------------------------------------------------------------------------
class Button : public Control, LayoutControl<layout::Button>
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

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	DialogEvent GetHandler() const { return handler; }

	void SetHandler(DialogEvent new_handler) { handler = new_handler; }

	string text;
	State state;
	int id;
	Texture* img;
	Int2 force_img_size;
	CustomButton* custom;
	bool hold;

private:
	DialogEvent handler;
};
