#include "Pch.h"
#include "TileGrid.h"

void TileGrid::Draw(ControlDrawData*)
{
	for(int y = 0; y < tileCount.x; ++y)
	{
		for(int x = 0; x < count_w; ++x)
		{
			Int2 shift(shift_x + x * 63, shift_y + y * 63);
			gui->DrawSprite(tItemBar, shift);
			uint index = x + y * count_w;
			if(index < ingredients.size())
			{
				pair<const Item*, uint>& p = ingredients[index];
				gui->DrawSprite(p.first->icon, Int2(shift + Int2(1)));
				if(p.second > 1u)
				{
					Rect rect3 = Rect::Create(Int2(shift_x + x * 63 + 2, shift_y + y * 63), Int2(64, 63));
					gui->DrawText(GameGui::font, Format("%d", p.second), DTF_BOTTOM, Color::Black, rect3);
				}
			}
		}
	}
}

void TileGrid::Event(GuiEvent event)
{
	if(event == GuiEvent_Initialize)// || event == GuiEvent_Resize)
	{
		tileSize = layout->texTile->GetSize().x - 1;
	}
	if(event == GuiEvent_Initialize || event == GuiEvent_Resize)
	{
		Int2 availableSize = size;
		availableSize.x -= 18;
		tileCount = Int2(availableSize.x / tileSize, availableSize.y / tileSize);
		const uint tileTotal = tileCount.x * tileCount.y;
		requireScrollbar = (items.size() > (uint)tileTotal);
		Int2 tilesSize = tileCount * tileSize;
		if(requireScrollbar)
			tilesSize.x += 18;
		offset = (size - tilesSize) / 2;
	}
}

void TileGrid::Update(float dt)
{

}
