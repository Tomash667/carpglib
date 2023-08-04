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
		Font* fontBig;
		Font* fontSmall;
	};
}

//-----------------------------------------------------------------------------
class TooltipController : public Control, public LayoutControl<layout::TooltipController>
{
public:
	typedef delegate<void(TooltipController*, int, int, bool)> Callback;

	TooltipController() : imgSize(Int2::Zero) {}
	void Draw() override;
	void Init(Callback getText);
	void Clear();
	void Refresh() { FormatBox(true); }
	void UpdateTooltip(float dt, int group, int id);

	string bigText, text, smallText;
	Texture* img;
	Int2 imgSize;
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
	Callback getText;
	float timer, alpha;
	Rect rBigText, rText, rSmallText;
};
