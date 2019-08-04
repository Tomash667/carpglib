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
	TextColor* text_color;
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
		Font* font;
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

	enum SelectionType
	{
		NONE,
		COLOR,
		BACKGROUND
	};

	Grid();

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Update(float dt) override;

	void Init();
	void Move(Int2& global_pos);
	void LostFocus() { scroll.LostFocus(); }
	void AddColumn(Type type, int width, cstring title = nullptr);
	void AddItem();
	void AddItems(int count);
	void RemoveItem(int id);
	void Reset();

	vector<Column> columns;
	int items, height, selected, total_width;
	GridEvent event;
	Scrollbar scroll;
	vector<Texture*> imgset;
	SelectionType selection_type;
	Color selection_color;
	SelectGridEvent select_event;
	bool single_line;
};
