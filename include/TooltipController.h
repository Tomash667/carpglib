// show box tooltip for elements under cursor
#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
class TooltipController;

//-----------------------------------------------------------------------------
typedef delegate<void(TooltipController*, int, int)> TooltipGetText;

//-----------------------------------------------------------------------------
namespace layout
{
	struct TooltipController : public Control
	{
		AreaLayout box;
		Font* font;
		Font* font_big;
		Font* font_small;
	};
}

//-----------------------------------------------------------------------------
class TooltipController : public Control, public LayoutControl<layout::TooltipController>
{
public:
	void Draw(ControlDrawData* cdd = nullptr) override;

	void Init(TooltipGetText get_text);
	void Clear();
	void Refresh() { FormatBox(); }
	void UpdateTooltip(float dt, int group, int id);

	string big_text, text, small_text;
	Texture* img;
	bool anything;

private:
	enum class State
	{
		NOT_VISIBLE,
		COUNTING,
		VISIBLE
	};

	void FormatBox();

	State state;
	int group, id;
	TooltipGetText get_text;
	float timer, alpha;
	Rect r_big_text, r_text, r_small_text;
};
