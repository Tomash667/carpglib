#pragma once

#include "Layout.h"
#include "Scrollbar.h"

namespace layout
{
	struct TileGrid : public Control
	{
		Texture* texTile;
	};
}

class TileGrid : public Control, public LayoutControl<layout::TileGrid>
{
public:
	struct Item
	{

	};

	void Draw(ControlDrawData* cdd = nullptr) override;
	void Event(GuiEvent event) override;
	void Update(float dt) override;

private:
	int tileSize;
	Int2 tileCount, offset;
	vector<Texture*> items;
	bool requireScrollbar;
};
