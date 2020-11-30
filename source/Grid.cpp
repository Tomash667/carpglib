#include "Pch.h"
#include "Grid.h"

#include "Input.h"

//=================================================================================================
Grid::Grid() : items(0), height(20), selected(-1), selection_type(COLOR), selection_color(Color::White), single_line(false), select_event(nullptr)
{
}

//=================================================================================================
void Grid::Draw(ControlDrawData*)
{
	int x = global_pos.x, y = global_pos.y;
	Rect r = { 0,y,0,y + height };

	// box i nag³ówki
	for(const Column& column : columns)
	{
		// box nag³ówka
		gui->DrawArea(Box2d::Create(Int2(x, y), Int2(column.width, height)), layout->box);
		// box zawartoœci
		gui->DrawArea(Box2d::Create(Int2(x, y + height), Int2(column.width, size.y - height)), layout->box);
		// tekst nag³ówka
		if(!column.title.empty())
		{
			r.Left() = x;
			r.Right() = x + column.width;
			gui->DrawText(layout->font, column.title, DTF_CENTER | DTF_VCENTER, Color::Black, r, &r);
		}
		x += column.width;
	}

	// zawartoœæ
	Cell cell;
	int clip_state;
	y += height - int(scroll.offset);
	const int clip_y[4] = { global_pos.y, global_pos.y + height, global_pos.y + size.y - height, global_pos.y + size.y };
	Rect clip_r;

	uint text_flags = DTF_CENTER | DTF_VCENTER;
	if(single_line)
		text_flags |= DTF_SINGLELINE;

	for(int i = 0; i < items; ++i)
	{
		int n = 0;
		x = global_pos.x;

		// ustal przycinanie komórek
		if(y < clip_y[0])
		{
			y += height;
			continue;
		}
		else if(y < clip_y[1])
			clip_state = 1;
		else if(y > clip_y[3])
			break;
		else if(y > clip_y[2])
			clip_state = 2;
		else
			clip_state = 0;

		// zaznaczenie t³a
		if(i == selected && selection_type == BACKGROUND)
		{
			Rect r2 = { x, y, x + total_width, y + height };
			if(clip_state == 1)
				r2.Top() = global_pos.y + height;
			else if(clip_state == 2)
				r2.Bottom() = global_pos.y + size.y;
			if(r2.Top() < r2.Bottom())
				gui->DrawArea(selection_color, r2);
		}

		for(const Column& column : columns)
		{
			Rect* clipping = nullptr;
			if(clip_state != 0)
			{
				clipping = &clip_r;
				clip_r.Left() = 0;
				clip_r.Right() = gui->wnd_size.x;
				if(clip_state == 1)
				{
					clip_r.Top() = global_pos.y + height;
					clip_r.Bottom() = gui->wnd_size.y;
				}
				else
				{
					clip_r.Top() = 0;
					clip_r.Bottom() = global_pos.y + size.y;
				}
			}

			switch(column.type)
			{
			case TEXT:
				{
					cell.color = Color::Black;
					if(selection_type == COLOR && i == selected)
						cell.color = selection_color;
					event(i, n, cell);

					r = Rect(x, y, x + column.width, y + height);
					gui->DrawText(layout->font, cell.text, text_flags, cell.color, r, clipping);
				}
				break;

			case IMG:
				{
					event(i, n, cell);

					const Int2 imgSize = cell.img->GetSize();
					if(column.size == Int2::Zero)
						gui->DrawSprite(cell.img, Int2(x + (column.width - imgSize.x) / 2, y + (height - imgSize.y) / 2), Color::White, clipping);
					else
					{
						const Vec2 scale = Texture::GetScale(imgSize, column.size);
						const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f,
							&Vec2(float(x + (column.width - column.size.x) / 2), float(y + (height - column.size.y) / 2)));
						gui->DrawSprite2(cell.img, mat, nullptr, clipping, Color::White);
					}
				}
				break;

			case IMG_TEXT:
				{
					cell.color = Color::Black;
					if(selection_type == COLOR && i == selected)
						cell.color = selection_color;
					event(i, n, cell);

					// calculate image & text pos
					const int textWidth = layout->font->CalculateSize(cell.text).x;
					const Int2 imgSize = cell.img->GetSize();
					const Int2 requiredSize = column.size == Int2::Zero ? imgSize : column.size;
					int totalWidth = requiredSize.x + 2 + textWidth;
					int imgOffset = (column.width - totalWidth) / 2;
					if(imgOffset < 1)
						imgOffset = 1;

					// image
					if(column.size == Int2::Zero)
						gui->DrawSprite(cell.img, Int2(x + imgOffset, y + (height - imgSize.y) / 2), Color::White, clipping);
					else
					{
						const Vec2 scale = Texture::GetScale(imgSize, column.size);
						const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f,
							&Vec2(float(x + imgOffset), float(y + (height - column.size.y) / 2)));
						gui->DrawSprite2(cell.img, mat, nullptr, clipping, Color::White);
					}

					// text
					r = Rect(x + imgOffset + requiredSize.x + 2, y, x + column.width, y + height);
					gui->DrawText(layout->font, cell.text, DTF_LEFT, cell.color, r, clipping);
				}
				break;

			case IMGSET:
				{
					cell.imgset = &imgset;
					imgset.clear();
					event(i, n, cell);
					if(!imgset.empty())
					{
						int img_total_width = 16 * imgset.size();
						int y2 = y + (height - 16) / 2;
						int dist, startx;
						if(img_total_width > column.width && imgset.size() > 1)
						{
							dist = 16 - (img_total_width - column.width) / (imgset.size() - 1);
							startx = 0;
						}
						else
						{
							dist = 16;
							startx = (column.width - img_total_width) / 2;
						}

						int x2 = x + startx;
						for(uint j = 0; j < imgset.size(); ++j)
						{
							gui->DrawSprite(imgset[j], Int2(x2, y2), Color::White, clipping);
							x2 += dist;
						}
					}
				}
				break;
			}
			x += column.width;
			++n;
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
		if(gui->cursor_pos.x >= global_pos.x && gui->cursor_pos.x < global_pos.x + total_width
			&& gui->cursor_pos.y >= global_pos.y + height && gui->cursor_pos.y < global_pos.y + size.y)
		{
			int n = (gui->cursor_pos.y - (global_pos.y + height) + int(scroll.offset)) / height;
			if(n >= 0 && n < items)
			{
				if(selection_type != NONE)
				{
					gui->cursor_mode = CURSOR_HOVER;
					if(input->PressedRelease(Key::LeftButton))
						selected = n;
				}
				if(select_event)
				{
					int y = gui->cursor_pos.x - global_pos.x, ysum = 0;
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
						gui->cursor_mode = CURSOR_HOVER;
						if(input->PressedRelease(Key::LeftButton))
							select_event(n, col, 0);
						else if(input->PressedRelease(Key::RightButton))
							select_event(n, col, 1);
					}
				}
			}
		}

		if(IsInside(gui->cursor_pos))
			scroll.ApplyMouseWheel();
		scroll.mouse_focus = mouse_focus;
		scroll.Update(dt);
	}
}

//=================================================================================================
void Grid::Init()
{
	scroll.pos = Int2(size.x - 16, height);
	scroll.size = Int2(16, size.y - height);
	scroll.total = height * items;
	scroll.part = scroll.size.y;
	scroll.offset = 0;

	total_width = 0;
	for(const Column& column : columns)
		total_width += column.width;
}

//=================================================================================================
void Grid::Move(Int2& _global_pos)
{
	global_pos = _global_pos + pos;
	scroll.global_pos = global_pos + scroll.pos;
}

//=================================================================================================
Grid::Column& Grid::AddColumn(Type type, int width, cstring title)
{
	Column& c = Add1(columns);
	c.type = type;
	c.width = width;
	if(title)
		c.title = title;
	c.size = Int2::Zero;
	return c;
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
void Grid::RemoveItem(int id)
{
	if(selected == id)
		selected = -1;
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

//=================================================================================================
// Return cell(row, column) under cursor, don't return header row
Int2 Grid::GetCell(const Int2& _pos) const
{
	Int2 p = _pos - global_pos;
	if(p.x < 0 || p.y < 0 || p.x >= size.x || p.y >= size.y)
		return Int2(-1, -1);

	int row = (p.y - int(scroll.offset)) / height;
	if(row == 0 || row > items)
		return Int2(-1, -1);

	--row;

	int col = -1, index = 0, x = 0;
	for(const Column& column : columns)
	{
		if(p.x < x + column.width)
		{
			col = index;
			break;
		}
		index++;
		x += column.width;
	}

	return Int2(row, col);
}

//=================================================================================================
int Grid::GetImgIndex(const Int2& _pos, const Int2& cellPt) const
{
	if(cellPt.x < 0 || cellPt.x >= items || cellPt.y < 0 || cellPt.y >= (int)columns.size())
		return -1;

	int x = 0;
	for(int i = 0; i < cellPt.y; ++i)
		x += columns[i].width;
	int y = (cellPt.x + 1) * height;
	Int2 p = _pos - global_pos;
	const Column& column = columns[cellPt.y];
	switch(column.type)
	{
	case TEXT:
		return -1;

	case IMG:
		{
			Cell cell;
			event(cellPt.x, cellPt.y, cell);

			const Int2 requiredSize = column.size != Int2::Zero ? column.size : cell.img->GetSize();
			const int cx = x + (column.width - requiredSize.x) / 2;
			const int cy = y + (height - requiredSize.y) / 2;
			return (p.x >= cx && p.x < cx + requiredSize.x && p.y >= cy && p.y < cy + requiredSize.y) ? 0 : -1;
		}
		break;

	case IMG_TEXT:
		{
			Cell cell;
			event(cellPt.x, cellPt.y, cell);

			const int textWidth = layout->font->CalculateSize(cell.text).x;
			const Int2 requiredSize = column.size == Int2::Zero ? cell.img->GetSize() : column.size;
			int totalWidth = requiredSize.x + 2 + textWidth;
			int imgOffset = (column.width - totalWidth) / 2;
			if(imgOffset < 1)
				imgOffset = 1;

			const int cx = x + imgOffset;
			const int cy = y + (height - requiredSize.y) / 2;
			return (p.x >= cx && p.x < cx + requiredSize.x && p.y >= cy && p.y < cy + requiredSize.y) ? 0 : -1;
		}

	case IMGSET:
		{
			Cell cell;
			cell.imgset = &imgset;
			imgset.clear();
			event(cellPt.x, cellPt.y, cell);
			if(!imgset.empty())
			{
				int img_total_width = 16 * imgset.size();
				int y2 = y + (height - 16) / 2;
				int dist, startx;
				if(img_total_width > column.width && imgset.size() > 1)
				{
					dist = 16 - (img_total_width - column.width) / (imgset.size() - 1);
					startx = 0;
				}
				else
				{
					dist = 16;
					startx = (column.width - img_total_width) / 2;
				}

				int x2 = x + startx;
				for(uint j = 0; j < imgset.size(); ++j)
				{
					if(p.x >= x2 && p.x < x2 + 16 && p.y >= y2 && p.y < y2 + 16)
						return j;
					x2 += dist;
				}
			}
			return -1;
		}

	default:
		assert(0);
		return -1;
	}
}
