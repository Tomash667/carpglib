#include "Pch.h"
#include "Grid.h"

#include "Input.h"

//=================================================================================================
Grid::Grid(bool isNew) : Control(isNew), items(0), height(20), selected(-1), selection_type(COLOR), selection_color(Color::White), single_line(false),
select_event(nullptr)
{
}

//=================================================================================================
void Grid::Draw(ControlDrawData*)
{
	int x = global_pos.x, y = global_pos.y;
	Rect r = { 0,y,0,y + height };

	// box i nag��wki
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
	{
		// box nag��wka
		gui->DrawArea(Box2d::Create(Int2(x, y), Int2(it->width, height)), layout->box);
		// box zawarto�ci
		gui->DrawArea(Box2d::Create(Int2(x, y + height), Int2(it->width, size.y - height)), layout->box);
		// tekst nag��wka
		if(!it->title.empty())
		{
			r.Left() = x;
			r.Right() = x + it->width;
			gui->DrawText(layout->font, it->title, DTF_CENTER | DTF_VCENTER, Color::Black, r, &r);
		}
		x += it->width;
	}

	// zawarto��
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

		// ustal przycinanie kom�rek
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

		// zaznaczenie t�a
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
					cell.text_color = &tc;
					event(i, n, cell);
					text = tc.text;
					color = tc.color;
				}

				if(selection_type == COLOR && i == selected)
					color = selection_color;

				r = Rect(x, y, x + it->width, y + height);

				if(clip_state == 0)
					gui->DrawText(layout->font, text, text_flags, color, r, &r);
				else
				{
					clip_r.Left() = r.Left();
					clip_r.Right() = r.Right();
					if(clip_state == 1)
					{
						clip_r.Top() = global_pos.y + height;
						clip_r.Bottom() = r.Bottom();
					}
					else
					{
						clip_r.Top() = r.Top();
						clip_r.Bottom() = global_pos.y + size.y;
					}
					gui->DrawText(layout->font, text, text_flags, color, r, &clip_r);
				}
			}
			else if(it->type == IMG)
			{
				event(i, n, cell);
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

				gui->DrawSprite(cell.img, Int2(x + (it->width - 16) / 2, y + (height - 16) / 2), Color::White, clipping);
			}
			else //if(it->type == IMGSET)
			{
				cell.imgset = &imgset;
				imgset.clear();
				event(i, n, cell);
				if(!imgset.empty())
				{
					int img_total_width = 16 * imgset.size();
					int y2 = y + (height - 16) / 2;
					int dist, startx;
					if(img_total_width > it->width && imgset.size() > 1)
					{
						dist = 16 - (img_total_width - it->width) / (imgset.size() - 1);
						startx = 0;
					}
					else
					{
						dist = 16;
						startx = (it->width - img_total_width) / 2;
					}

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

					int x2 = x + startx;
					for(uint j = 0; j < imgset.size(); ++j)
					{
						gui->DrawSprite(imgset[j], Int2(x2, y2), Color::White, clipping);
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
	if((is_new && mouse_focus) || (!is_new && input->Focus() && focus))
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
void Grid::Event(GuiEvent e)
{
	if(e == GuiEvent_Initialize && is_new)
		Init();
}

//=================================================================================================
void Grid::Init()
{
	scroll.pos = Int2(size.x - 16, height);
	scroll.size = Int2(16, size.y - height);
	scroll.total = height * items;
	scroll.part = scroll.size.y;
	scroll.offset = 0;
	if(is_new)
		scroll.global_pos = global_pos + scroll.pos;

	total_width = 0;
	for(vector<Column>::iterator it = columns.begin(), end = columns.end(); it != end; ++it)
		total_width += it->width;
}

//=================================================================================================
void Grid::Move(Int2& _global_pos)
{
	global_pos = _global_pos + pos;
	scroll.global_pos = global_pos + scroll.pos;
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
