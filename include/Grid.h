#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"
#include "Scrollbar.h"

//-----------------------------------------------------------------------------
struct Cell
{
	cstring text;
	Color color;
	vector<Texture*>* imgset;
	Texture* img;
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
		IMG,
		IMG_TEXT,
		IMGSET
	};

	struct Column
	{
		Type type;
		int width;
		string title;
		Int2 size;
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
	Column& AddColumn(Type type, int width, cstring title = nullptr);
	void AddItem();
	void AddItems(int count);
	void RemoveItem(int id);
	void Reset();
	Int2 GetCell(const Int2& pos) const;
	int GetImgIndex(const Int2& pos, const Int2& cell) const;

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
