#include "Pch.h"
#include "ListBox.h"

#include "Input.h"
#include "MenuList.h"
#include "MenuStrip.h"

//=================================================================================================
ListBox::ListBox(bool is_new) : Control(is_new), scrollbar(false, is_new), selected(-1), event_handler(nullptr), event_handler2(nullptr), menu(nullptr), menu_strip(nullptr),
forceImgSize(0, 0), itemHeight(20), requireScrollbar(false), textFlags(DTF_SINGLELINE)
{
	SetOnCharHandler(true);
}

//=================================================================================================
ListBox::~ListBox()
{
	DeleteElements(items);
	delete menu;
	delete menu_strip;
}

//=================================================================================================
void ListBox::Draw(ControlDrawData*)
{
	if(collapsed)
	{
		// box
		gui->DrawArea(Box2d::Create(global_pos, size), layout->box);

		// element
		if(selected != -1)
		{
			Rect rc = { global_pos.x + 2, global_pos.y + 2, global_pos.x + size.x - 12, global_pos.y + size.y - 2 };
			gui->DrawText(layout->font, items[selected]->ToString(), DTF_SINGLELINE, Color::Black, rc, &rc);
		}

		// image
		gui->DrawSprite(layout->down_arrow, Int2(global_pos.x + size.x - 10, global_pos.y + (size.y - 10) / 2));
	}
	else
	{
		// box
		gui->DrawArea(Box2d::Create(global_pos, realSize), layout->box);

		// selection
		Rect rc = { global_pos.x, global_pos.y, global_pos.x + realSize.x, global_pos.y + realSize.y };
		if(selected != -1)
		{
			int offsetY;
			if(itemHeight == -1)
			{
				offsetY = 0;
				for(int i = 0; i < selected; ++i)
					offsetY += items[i]->height;
			}
			else
				offsetY = selected * itemHeight;

			Rect rs = { global_pos.x + 2, global_pos.y - int(scrollbar.offset) + 2 + offsetY, global_pos.x + realSize.x - 2, 0 };
			rs.Bottom() = rs.Top() + items[selected]->height;
			Rect out;
			if(Rect::Intersect(rs, rc, out))
				gui->DrawArea(Box2d(out), layout->selection);
		}

		// elements
		Rect r;
		r.Right() = global_pos.x + realSize.x - 2;
		r.Top() = global_pos.y - int(scrollbar.offset) + 2;
		r.Bottom() = r.Top() + itemHeight;
		int orig_x = global_pos.x + 2;
		for(GuiElement* e : items)
		{
			r.Bottom() = r.Top() + e->height;
			if(e->tex)
			{
				Int2 required_size = forceImgSize, img_size;
				Vec2 scale;
				e->tex->ResizeImage(required_size, img_size, scale);
				const Vec2 pos((float)orig_x, float(r.Top() + (e->height - required_size.y) / 2));
				const Matrix mat = Matrix::Transform2D(nullptr, 0.f, &scale, nullptr, 0.f, &pos);
				gui->DrawSprite2(e->tex, mat, nullptr, &rc, Color::White);
				r.Left() = orig_x + required_size.x;
			}
			else
				r.Left() = orig_x;
			if(!gui->DrawText(layout->font, e->ToString(), textFlags, Color::Black, r, &rc))
				break;
			r.Top() += e->height;
		}

		// scrollbar
		if(requireScrollbar)
			scrollbar.Draw();
	}
}

//=================================================================================================
void ListBox::Update(float dt)
{
	if(collapsed)
	{
		if(is_new)
		{
			if(mouse_focus && input->PressedRelease(Key::LeftButton))
			{
				if(menu_strip->IsOpen())
					TakeFocus(true);
				else
				{
					menu_strip->ShowMenu(global_pos + Int2(0, size.y));
					menu_strip->SetSelectedIndex(selected);
				}
			}
		}
		else
		{
			if(menu->visible)
			{
				if(!menu->focus)
					menu->visible = false;
			}
			else if(mouse_focus && input->Focus() && PointInRect(gui->cursor_pos, global_pos, size) && input->PressedRelease(Key::LeftButton))
			{
				menu->global_pos = global_pos + Int2(0, size.y);
				if(menu->global_pos.y + menu->size.y >= gui->wnd_size.y)
				{
					menu->global_pos.y = global_pos.y - menu->size.y;
					if(menu->global_pos.y < 0)
						menu->global_pos.y = gui->wnd_size.y - menu->size.y;
				}
				menu->visible = true;
				menu->focus = true;
			}
		}
	}
	else
	{
		if(focus && selected != -1)
		{
			int newSelected = -1;
			if(input->DownRepeat(Key::Up))
				newSelected = selected - 1;
			else if(input->DownRepeat(Key::Down))
				newSelected = selected + 1;
			else if(input->Pressed(Key::PageUp))
				newSelected = 0;
			else if(input->Pressed(Key::PageDown))
				newSelected = items.size() - 1;

			if(newSelected != selected && newSelected >= 0 && newSelected < (int)items.size())
				ChangeIndexEvent(newSelected, false, true);
		}

		if(is_new && requireScrollbar)
		{
			if(mouse_focus)
				scrollbar.ApplyMouseWheel();
			UpdateControl(&scrollbar, dt);
		}

		if(mouse_focus && input->Focus() && PointInRect(gui->cursor_pos, global_pos, realSize))
		{
			int bt = 0;

			if(event_handler2 && gui->DoubleClick(Key::LeftButton))
			{
				int new_index = PosToIndex(gui->cursor_pos.y);
				if(new_index != -1 && new_index == selected)
				{
					event_handler2(A_DOUBLE_CLICK, selected);
					bt = -1;
				}
			}

			if(bt != -1)
			{
				if(input->PressedRelease(Key::LeftButton))
					bt = 1;
				else if(input->PressedRelease(Key::RightButton))
					bt = 2;
			}

			if(bt > 0)
			{
				int new_index = PosToIndex(gui->cursor_pos.y);
				bool ok = true;
				if(new_index != -1 && new_index != selected)
					ok = ChangeIndexEvent(new_index, false, false);

				if(bt == 2 && menu_strip && ok)
				{
					if(event_handler2)
					{
						if(!event_handler2(A_BEFORE_MENU_SHOW, new_index))
							ok = false;
					}
					if(ok)
					{
						menu_strip->SetHandler(delegate<void(int)>(this, &ListBox::OnSelect));
						menu_strip->ShowMenu();
					}
				}
				else if(is_new)
					TakeFocus(true);
			}
		}

		if(!is_new)
		{
			if(IsInside(gui->cursor_pos))
				scrollbar.ApplyMouseWheel();
			scrollbar.mouse_focus = mouse_focus;
			scrollbar.Update(dt);
		}
	}
}

//=================================================================================================
void ListBox::Event(GuiEvent e)
{
	if(e == GuiEvent_Moved)
	{
		if(!is_new)
			global_pos = parent->global_pos + pos;
		scrollbar.global_pos = global_pos + scrollbar.pos;
	}
	else if(e == GuiEvent_Initialize)
	{
		realSize = size;
		scrollbar.pos = Int2(size.x - 16, 0);
		scrollbar.size = Int2(16, size.y);
		scrollbar.offset = 0.f;
		scrollbar.total = CalculateItemsHeight();
		scrollbar.part = size.y - 4;

		if(collapsed)
		{
			if(is_new)
			{
				menu_strip = new MenuStrip(items, size.x);
				menu_strip->SetHandler(DialogEvent(this, &ListBox::OnSelect));
			}
			else
			{
				menu = new MenuList;
				menu->AddItems(items, false);
				menu->visible = false;
				menu->Init();
				menu->size.x = size.x;
				menu->event_handler = DialogEvent(this, &ListBox::OnSelect);
			}
		}

		UpdateScrollbarVisibility();
	}
	else if(e == GuiEvent_Resize)
	{
		scrollbar.pos = Int2(size.x - 16, 0);
		scrollbar.size = Int2(16, size.y);
		scrollbar.offset = 0.f;
		scrollbar.total = CalculateItemsHeight();
		scrollbar.part = size.y - 4;
		scrollbar.global_pos = global_pos + scrollbar.pos;

		UpdateScrollbarVisibility();
	}
}

//=================================================================================================
void ListBox::OnChar(char c)
{
	if(c >= 'A' && c <= 'Z')
		c = tolower(c);

	bool first = true;
	int start_index = selected;
	if(start_index == -1)
		start_index = 0;
	int index = start_index;

	while(true)
	{
		auto item = items[index];
		char starts_with = tolower(item->ToString()[0]);
		if(starts_with == c)
		{
			if(index == selected && first)
				first = false;
			else
			{
				ChangeIndexEvent(index, false, true);
				return;
			}
		}
		else if(index == start_index)
		{
			if(first)
				first = false;
			else
				return;
		}
		index = (index + 1) % items.size();
	}
}

//=================================================================================================
void ListBox::Add(GuiElement* e)
{
	assert(e);
	CalculateItemHeight(e);
	items.push_back(e);
	scrollbar.total += e->height;
	UpdateScrollbarVisibility();

	if(menu)
		menu->AddItem(e);
}

//=================================================================================================
void ListBox::ScrollTo(int index, bool center)
{
	const int count = (int)items.size();
	assert(index >= 0 && index < count);
	if(itemHeight == -1)
	{
		const int selectedItemHeight = items[index]->height;
		int posy = 0;
		for(int i = 0; i < index; ++i)
			posy += items[i]->height;

		if(center)
		{
			if(scrollbar.part >= scrollbar.total)
				scrollbar.offset = 0.f;
			else
			{
				scrollbar.offset = float(posy - (scrollbar.part - selectedItemHeight) / 2);
				if((int)scrollbar.offset + scrollbar.part > scrollbar.total)
					scrollbar.offset = float(scrollbar.total - scrollbar.part);
			}
		}
		else
		{
			const int offsety = posy - (int)scrollbar.offset;
			if(offsety < 0)
				scrollbar.offset = (float)posy;
			else if(offsety + selectedItemHeight > scrollbar.part)
				scrollbar.offset = float(selectedItemHeight + posy - scrollbar.part);
		}
	}
	else
	{
		if(center)
		{
			const int n = scrollbar.part / itemHeight;
			if(index < n)
				scrollbar.offset = 0.f;
			else if(index > count - n)
				scrollbar.offset = float(scrollbar.total - scrollbar.part);
			else
				scrollbar.offset = float((index - n) * itemHeight);
		}
		else
		{
			const int posy = index * itemHeight;
			const int offsety = posy - (int)scrollbar.offset;
			if(offsety < 0)
				scrollbar.offset = (float)posy;
			else if(offsety + itemHeight > scrollbar.part)
				scrollbar.offset = float(itemHeight + posy - scrollbar.part);
		}
	}
}

//=================================================================================================
void ListBox::OnSelect(int index)
{
	if(is_new)
	{
		if(collapsed)
		{
			if(index != selected)
				ChangeIndexEvent(index, false, false);
		}
		else
		{
			if(event_handler2)
				event_handler2(A_MENU, index);
		}
	}
	else
	{
		menu->visible = false;
		if(index != selected)
			ChangeIndexEvent(index, false, false);
	}
}

//=================================================================================================
void ListBox::Sort()
{
	std::sort(items.begin(), items.end(),
		[](GuiElement* e1, GuiElement* e2)
	{
		return strcoll(e1->ToString(), e2->ToString()) < 0;
	});
}

//=================================================================================================
GuiElement* ListBox::Find(int value)
{
	for(GuiElement* e : items)
	{
		if(e->value == value)
			return e;
	}

	return nullptr;
}

//=================================================================================================
int ListBox::FindIndex(int value)
{
	for(int i = 0; i < (int)items.size(); ++i)
	{
		if(items[i]->value == value)
			return i;
	}

	return -1;
}

//=================================================================================================
void ListBox::Select(int index, bool send_event)
{
	if(send_event)
	{
		if(!ChangeIndexEvent(index, false, false))
			return;
	}
	else
		selected = index;
	ScrollTo(index);
}

//=================================================================================================
void ListBox::Select(delegate<bool(GuiElement*)> pred, bool send_event)
{
	int index = 0;
	for(GuiElement* item : items)
	{
		if(pred(item))
		{
			if(selected != index)
			{
				selected = index;
				if(event_handler)
					event_handler(selected);
				if(event_handler2)
					event_handler2(A_INDEX_CHANGED, selected);
			}
			break;
		}
		++index;
	}
}

//=================================================================================================
void ListBox::ForceSelect(int index)
{
	ChangeIndexEvent(index, true, true);
	ScrollTo(index);
}

//=================================================================================================
void ListBox::Insert(GuiElement* e, int index)
{
	assert(e && index >= 0);
	if(index >= (int)items.size())
	{
		Add(e);
		return;
	}
	CalculateItemHeight(e);
	items.insert(items.begin() + index, e);
	scrollbar.total += e->height;
	if(selected >= index)
		++selected;
	UpdateScrollbarVisibility();
}

//=================================================================================================
void ListBox::Remove(int index)
{
	assert(index >= 0 && index < (int)items.size());
	delete items[index];
	items.erase(items.begin() + index);
	if(selected > index || (selected == index && selected == (int)items.size()))
		--selected;
	if(event_handler)
		event_handler(selected);
	if(event_handler2)
		event_handler2(A_INDEX_CHANGED, selected);
	UpdateScrollbarVisibility();
}

//=================================================================================================
int ListBox::PosToIndex(int y)
{
	const int realY = (y - global_pos.y + int(scrollbar.offset));
	int index;
	if(itemHeight == -1)
	{
		index = -1;
		int total = 0;
		int tmpIndex = 0;
		for(GuiElement* e : items)
		{
			if(realY >= total && realY < total + e->height)
			{
				index = tmpIndex;
				break;
			}
			total += e->height;
			++tmpIndex;
		}
	}
	else
		index = realY / itemHeight;
	if(index >= 0 && index < (int)items.size())
		return index;
	else
		return -1;
}

//=================================================================================================
bool ListBox::ChangeIndexEvent(int index, bool force, bool scrollTo)
{
	if(!force && event_handler2)
	{
		if(!event_handler2(A_BEFORE_CHANGE_INDEX, index))
			return false;
	}

	selected = index;

	if(event_handler)
		event_handler(selected);
	if(event_handler2)
		event_handler2(A_INDEX_CHANGED, selected);

	if(scrollTo && !collapsed)
		ScrollTo(index);

	return true;
}

//=================================================================================================
void ListBox::Reset()
{
	scrollbar.offset = 0.f;
	scrollbar.total = 0;
	selected = -1;
	DeleteElements(items);
	UpdateScrollbarVisibility();
	if(menu)
		menu->Reset();
}

//=================================================================================================
void ListBox::UpdateScrollbarVisibility()
{
	if(!initialized)
		return;
	requireScrollbar = scrollbar.IsRequired();
	if(requireScrollbar)
		realSize = Int2(size.x - 15, size.y);
	else
		realSize = size;
}

//=================================================================================================
void ListBox::CalculateItemHeight(GuiElement* e)
{
	if(itemHeight == -1)
	{
		Int2 required_size = forceImgSize, img_size;
		Vec2 scale;
		e->tex->ResizeImage(required_size, img_size, scale);

		int allowedWidth = size.x - 19 - required_size.x; // remove scrollbar, padding & image width
		e->height = layout->font->CalculateSize(e->ToString(), allowedWidth).y + layout->auto_padding;
	}
	else
		e->height = itemHeight;
}

//=================================================================================================
int ListBox::CalculateItemsHeight()
{
	if(itemHeight == -1)
		return itemHeight * items.size();
	else
	{
		int height = 0;
		for(GuiElement* e : items)
			height += e->height;
		return height;
	}
}
