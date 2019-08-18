// show box tooltip for elements under cursor
#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

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
	typedef delegate<void(TooltipController*, int, int, bool)> Callback;

	void Draw(ControlDrawData* cdd = nullptr) override;

	void Init(Callback get_text);
	void Clear();
	void Refresh() { FormatBox(true); }
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

	void FormatBox(bool refresh);

	State state;
	int group, id;
	Callback get_text;
	float timer, alpha;
	Rect r_big_text, r_text, r_small_text;
};
