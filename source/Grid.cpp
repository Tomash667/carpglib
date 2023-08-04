#include "Pch.h"
#include "Grid.h"

#include "Input.h"

//=================================================================================================
Grid::Grid() : items(0), height(20), selected(-1), allowSelect(true), singleLine(false), selectEvent(nullptr)
{
}

//=================================================================================================
void Grid::Draw()
{
	const int headerHeight = height + layout->border * 2;
	int x = globalPos.x, y = globalPos.y;
	Rect r = { 0, y, 0, y + headerHeight };

	// box i nag³ówki
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
	{
		// box nag³ówka
		gui->DrawArea(Box2d::Create(Int2(x, y), Int2(it->width, headerHeight)), layout->box);
		// box zawartoœci
		gui->DrawArea(Box2d::Create(Int2(x, y + headerHeight), Int2(it->width, size.y - headerHeight)), layout->box);
		// tekst nag³ówka
		if(!it->title.empty())
		{
			r.Left() = x;
			r.Right() = x + it->width;
			gui->DrawText(layout->font, it->title, DTF_CENTER | DTF_VCENTER, Color::Black, r, &r);
		}
		x += it->width;
	}

	// zawartoœæ
	const Rect clipRect = { globalPos.x + layout->border, globalPos.y + headerHeight + layout->border,
		globalPos.x + size.x - layout->border, globalPos.y + size.y - layout->border };
	Cell cell;
	y = clipRect.Top() - int(scroll.offset);

	uint textFlags = DTF_CENTER | DTF_VCENTER;
	if(singleLine)
		textFlags |= DTF_SINGLELINE;

	for(int i = 0; i < items; ++i)
	{
		if(y < clipRect.Top() - height)
		{
			// row above visible region
			y += height;
			continue;
		}
		else if(y > clipRect.Bottom() + height)
		{
			// row below visible region
			break;
		}

		int n = 0;
		x = globalPos.x;

		// zaznaczenie t³a
		if(i == selected && allowSelect)
		{
			Rect r2 = { x + layout->border, y, x + totalWidth - layout->border, y + height };
			Box2d clip(clipRect);
			gui->DrawArea(Box2d(r2), layout->selection, &clip);
		}

		for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it, ++n)
		{
			if(it->type == TEXT || it->type == TEXT_COLOR)
			{
				Color color;
				cstring text;

				if(it->type == TEXT)
				{
					color = Color::Black;
					event(i, n, cell);
					text = cell.text;
				}
				else
				{
					TextColor tc;
					cell.textColor = &tc;
					event(i, n, cell);
					text = tc.text;
					color = tc.color;
				}

				r = Rect(x, y, x + it->width, y + height);
				gui->DrawText(layout->font, text, textFlags, color, r, &clipRect);
					}
			else if(it->type == IMG)
			{
				event(i, n, cell);
				gui->DrawSprite(cell.img, Int2(x + (it->width - 16) / 2, y + (height - 16) / 2), Color::White, &clipRect);
					}
			else //if(it->type == IMGSET)
			{
				cell.imgset = &imgset;
				imgset.clear();
				event(i, n, cell);
				if(!imgset.empty())
				{
					int imgTotalWidth = 16 * imgset.size();
					int y2 = y + (height - 16) / 2;
					int dist, startx;
					if(imgTotalWidth > it->width && imgset.size() > 1)
					{
						dist = 16 - (imgTotalWidth - it->width) / (imgset.size() - 1);
						startx = 0;
					}
					else
					{
						dist = 16;
						startx = (it->width - imgTotalWidth) / 2;
					}

					int x2 = x + startx;
					for(uint j = 0; j < imgset.size(); ++j)
					{
						gui->DrawSprite(imgset[j], Int2(x2, y2), Color::White, &clipRect);
						x2 += dist;
					}
				}
			}
			x += it->width;
		}
		y += height;
	}

	scroll.Draw();
}

//=================================================================================================
void Grid::Update(float dt)
{
	if(input->Focus() && focus)
	{
		const int headerHeight = height + layout->border * 2;
		if(gui->cursorPos.x >= globalPos.x && gui->cursorPos.x < globalPos.x + totalWidth
			&& gui->cursorPos.y >= globalPos.y + headerHeight && gui->cursorPos.y < globalPos.y + size.y)
		{
			int n = (gui->cursorPos.y - (globalPos.y + headerHeight) + int(scroll.offset)) / height;
			if(n >= 0 && n < items)
			{
				if(allowSelect)
				{
					gui->SetCursorMode(CURSOR_HOVER);
					if(input->PressedRelease(Key::LeftButton))
						selected = n;
				}
				if(selectEvent)
				{
					int y = gui->cursorPos.x - globalPos.x, ysum = 0;
					int col = -1;

					for(int i = 0; i < (int)columns.size(); ++i)
					{
						ysum += columns[i].width;
						if(y <= ysum)
						{
							col = i;
							break;
						}
					}

					// celowo nie zaznacza 1 kolumny!
					if(col > 0)
					{
						gui->SetCursorMode(CURSOR_HOVER);
						if(input->PressedRelease(Key::LeftButton))
							selectEvent(n, col, 0);
						else if(input->PressedRelease(Key::RightButton))
							selectEvent(n, col, 1);
					}
				}
			}
		}

		if(IsInside(gui->cursorPos))
			scroll.ApplyMouseWheel();
		scroll.mouseFocus = mouseFocus;
		scroll.Update(dt);
	}
}

//=================================================================================================
void Grid::Init()
{
	const int headerHeight = height + layout->border * 2;
	scroll.pos = Int2(size.x - 16, headerHeight);
	scroll.size = Int2(16, size.y - headerHeight);
	scroll.total = height * items;
	scroll.part = scroll.size.y - layout->border * 2;
	scroll.offset = 0;

	totalWidth = 0;
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
		totalWidth += it->width;
}

//=================================================================================================
void Grid::Move(const Int2& movePos)
{
	globalPos = movePos + pos;
	scroll.globalPos = globalPos + scroll.pos;
}

//=================================================================================================
void Grid::AddColumn(Type type, int width, cstring title)
{
	Column& c = Add1(columns);
	c.type = type;
	c.width = width;
	if(title)
		c.title = title;
}

//=================================================================================================
void Grid::AddItem()
{
	++items;
	scroll.total = items * height;
}

//=================================================================================================
void Grid::AddItems(int count)
{
	assert(count > 0);
	items += count;
	scroll.total = items * height;
}

//=================================================================================================
void Grid::RemoveItem(int id, bool keepSelection)
{
	if(selected == id)
	{
		if(keepSelection)
		{
			if(selected + 1 == items)
				--selected;
		}
		else
			selected = -1;
	}
	else if(selected > id)
		--selected;
	--items;
	scroll.total = items * height;
	const float s = float(scroll.total - scroll.part);
	if(scroll.offset > s)
		scroll.offset = s;
	if(scroll.part >= scroll.total)
		scroll.offset = 0;
}

//=================================================================================================
void Grid::Reset()
{
	items = 0;
	selected = -1;
	scroll.total = 0;
	scroll.part = size.y;
	scroll.offset = 0;
}
