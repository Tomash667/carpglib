#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct TextColor
{
	cstring text;
	Color color;
};

//-----------------------------------------------------------------------------
union Cell
{
	cstring text;
	vector<Texture*>* imgset;
	Texture* img;
	TextColor* textColor;
};

//-----------------------------------------------------------------------------
typedef delegate<void(int, int, Cell&)> GridEvent;
typedef delegate<void(int, int, int)> SelectGridEvent;

//-----------------------------------------------------------------------------
namespace layout
{
	struct Grid : public Control
	{
		AreaLayout box;
		AreaLayout selection;
		Font* font;
		int border;
	};
}

//-----------------------------------------------------------------------------
class Grid : public Control, public LayoutControl<layout::Grid>
{
public:
	enum Type
	{
		TEXT,
		TEXT_COLOR,
		IMG,
		IMGSET
	};

	struct Column
	{
		Type type;
		int width;
		string title;
	};

	Grid();

	void Draw() override;
	void Update(float dt) override;

	void Init();
	void Move(const Int2& movePos);
	void LostFocus() { scroll.LostFocus(); }
	void AddColumn(Type type, int width, cstring title = nullptr);
	void AddItem();
	void AddItems(int count);
	void RemoveItem(int id);
	void Reset();

	vector<Column> columns;
	int items, height, selected, totalWidth;
	GridEvent event;
	Scrollbar scroll;
	vector<Texture*> imgset;
	SelectGridEvent selectEvent;
	bool allowSelect, singleLine;
};
