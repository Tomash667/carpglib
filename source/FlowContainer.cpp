#include "Pch.h"
#include "FlowContainer.h"

#include "Input.h"

//-----------------------------------------------------------------------------
ObjectPool<FlowItem> FlowItem::Pool;

void FlowItem::Set(cstring _text)
{
	type = Section;
	text = _text;
	state = Button::NONE;
	group = -1;
	id = -1;
}

void FlowItem::Set(cstring _text, int _group, int _id)
{
	type = Item;
	text = _text;
	group = _group;
	id = _id;
	state = Button::NONE;
}

void FlowItem::Set(int _group, int _id, int _texId, bool disabled)
{
	type = Button;
	group = _group;
	id = _id;
	texId = _texId;
	state = (disabled ? Button::DISABLED : Button::NONE);
}

//=================================================================================================
FlowContainer::FlowContainer() : id(-1), group(-1), onButton(nullptr), buttonSize(0, 0), wordWrap(true), allowSelect(false), fill(false), selected(nullptr),
batchChanges(false), buttonTex(nullptr)
{
	size = Int2(-1, -1);
}

//=================================================================================================
FlowContainer::~FlowContainer()
{
	Clear();
}

//=================================================================================================
void FlowContainer::Update(float dt)
{
	bool ok = false;
	group = -1;
	id = -1;

	if(mouseFocus)
	{
		if(IsInside(gui->cursorPos))
		{
			ok = true;

			scroll.ApplyMouseWheel();

			Int2 off(0, (int)scroll.offset);
			bool haveButton = false;

			if(allowSelect)
			{
				for(FlowItem* fi : items)
				{
					if(fi->state == Button::HOVER)
						fi->state = Button::NONE;
				}
			}

			for(FlowItem* fi : items)
			{
				Int2 p = fi->pos - off + globalPos;
				if(fi->type == FlowItem::Item)
				{
					if(haveButton)
					{
						p.y -= 2;
						haveButton = false;
					}

					if(fi->group != -1 && Rect::IsInside(gui->cursorPos, p, fi->size))
					{
						group = fi->group;
						id = fi->id;
						if(allowSelect && fi->state != Button::DISABLED)
						{
							gui->SetCursorMode(CURSOR_HOVER);
							if(onSelect && input->Pressed(Key::LeftButton))
							{
								selected = fi;
								onSelect();
								return;
							}
							else if(fi->state == Button::NONE)
								fi->state = Button::HOVER;
						}
					}
				}
				else if(fi->type == FlowItem::Button && fi->state != Button::DISABLED)
				{
					haveButton = true;
					if(Rect::IsInside(gui->cursorPos, p, fi->size))
					{
						group = fi->group;
						id = fi->id;
						gui->SetCursorMode(CURSOR_HOVER);
						if(fi->state == Button::DOWN)
						{
							if(input->Up(Key::LeftButton))
							{
								fi->state = Button::HOVER;
								onButton(fi->group, fi->id);
								return;
							}
						}
						else if(input->Pressed(Key::LeftButton))
							fi->state = Button::DOWN;
						else
							fi->state = Button::HOVER;
					}
					else
						fi->state = Button::NONE;
				}
				else
					haveButton = false;
			}
		}

		scroll.mouseFocus = mouseFocus;
		scroll.Update(dt);
	}

	if(!ok)
	{
		for(FlowItem* fi : items)
		{
			if(fi->type == FlowItem::Button && fi->state != Button::DISABLED)
				fi->state = Button::NONE;
		}
	}
}

//=================================================================================================
void FlowContainer::Draw()
{
	gui->DrawArea(Box2d::Create(globalPos, size - Int2(16, 0)), layout->box);

	scroll.Draw();

	int sizex = size.x - 16;

	Rect rect;
	Rect clip = Rect::Create(globalPos, size, layout->border);
	int offset = (int)scroll.offset;
	uint flags = (wordWrap ? 0 : DTF_SINGLELINE) | DTF_PARSE_SPECIAL;

	for(FlowItem* fi : items)
	{
		if(fi->type != FlowItem::Button)
		{
			rect = Rect::Create(globalPos + fi->pos - Int2(0, offset), fi->size);

			// text above box
			if(rect.Bottom() < globalPos.y)
				continue;

			if(fi->state == Button::DOWN || fi->state == Button::HOVER)
			{
				Rect rs = { globalPos.x + 2, rect.Top(), globalPos.x + sizex, rect.Bottom() };
				Rect out;
				if(Rect::Intersect(rs, clip, out))
					gui->DrawArea(Box2d(out), fi->state == Button::DOWN ? layout->selection : layout->hover);
			}

			if(!gui->DrawText(fi->type == FlowItem::Section ? layout->fontSection : layout->font, fi->text, flags,
				(fi->state != Button::DISABLED ? Color::Black : Color(64, 64, 64)), rect, &clip))
				break;
		}
		else
		{
			// button above or below box
			if(globalPos.y + fi->pos.y - offset + fi->size.y < globalPos.y
				|| globalPos.y + fi->pos.y - offset > globalPos.y + size.y)
				continue;

			const AreaLayout& area = buttonTex[fi->texId].tex[fi->state];
			const Box2d clipRect(clip);
			gui->DrawArea(Box2d::Create(globalPos + fi->pos - Int2(0, offset), area.size), area, &clipRect);
		}
	}
}

//=================================================================================================
FlowItem* FlowContainer::Add()
{
	FlowItem* item = FlowItem::Pool.Get();
	items.push_back(item);
	return item;
}

//=================================================================================================
void FlowContainer::Clear()
{
	FlowItem::Pool.Free(items);
	batchChanges = false;
}

//=================================================================================================
// set group & index only if there is selection
void FlowContainer::GetSelected(int& group, int& id)
{
	if(this->group != -1)
	{
		group = this->group;
		id = this->id;
	}
}

//=================================================================================================
void FlowContainer::UpdateSize(const Int2& newPos, const Int2& newSize, bool isVisible)
{
	globalPos = pos = newPos;
	if(size.y != newSize.y && isVisible)
	{
		size = newSize;
		Reposition();
	}
	else
		size = newSize;
	scroll.globalPos = scroll.pos = globalPos + Int2(size.x - 17, 0);
	scroll.size = Int2(16, size.y);
	scroll.part = size.y;
}

//=================================================================================================
void FlowContainer::UpdatePos(const Int2& parentPos)
{
	globalPos = pos + parentPos;
	scroll.globalPos = scroll.pos = globalPos + Int2(size.x - 17, 0);
	scroll.size = Int2(16, size.y);
	scroll.part = size.y;
}

//=================================================================================================
void FlowContainer::Reposition()
{
	const int sizex = wordWrap ? (size.x - (layout->border + layout->padding) * 2 - scroll.size.x - 1) : 10000;
	const int shift = layout->border + layout->padding;
	int y = layout->border;
	bool haveButton = false;

	for(FlowItem* fi : items)
	{
		if(fi->type != FlowItem::Button)
		{
			if(fi->type != FlowItem::Section)
			{
				if(haveButton)
				{
					fi->size = layout->font->CalculateSize(fi->text, sizex - 2 - buttonSize.x);
					if(fill)
						fi->size.x = sizex - 2 - buttonSize.x;
					fi->pos = Int2(shift + 2 + buttonSize.x, y);
				}
				else
				{
					fi->size = layout->font->CalculateSize(fi->text, sizex);
					if(fill)
						fi->size.x = sizex;
					fi->pos = Int2(shift, y);
				}
			}
			else
			{
				fi->size = layout->fontSection->CalculateSize(fi->text, sizex);
				if(fill)
					fi->size.x = sizex;
				fi->pos = Int2(shift, y);
			}
			haveButton = false;
			y += fi->size.y + 1;
		}
		else
		{
			fi->size = buttonSize;
			fi->pos = Int2(shift, y);
			haveButton = true;
		}
	}

	scroll.total = y + layout->border - 1;
	if(scroll.offset + scroll.part > scroll.total)
		scroll.offset = max(0.f, float(scroll.total - scroll.part));
}

//=================================================================================================
FlowItem* FlowContainer::Find(int group, int id)
{
	for(FlowItem* fi : items)
	{
		if(fi->group == group && fi->id == id)
			return fi;
	}

	return nullptr;
}

//=================================================================================================
void FlowContainer::SetItems(vector<FlowItem*>& items)
{
	Clear();
	this->items = std::move(items);
	Reposition();
	ResetScrollbar();
}

//=================================================================================================
void FlowContainer::UpdateText(FlowItem* item, cstring text, bool batch)
{
	assert(item && text);

	item->text = text;

	const int sizex = wordWrap ? (size.x - (layout->border + layout->padding) * 2 - scroll.size.x - 1) : 10000;
	Int2 newSize = layout->font->CalculateSize(text, (item->pos.x == layout->border + layout->padding ? sizex : sizex - 2 - buttonSize.x));

	if(newSize.y != item->size.y)
	{
		item->size = newSize;

		if(batch)
			batchChanges = true;
		else
			UpdateText();
	}
	else
	{
		// only width changed, no need to recalculate positions
		item->size = newSize;
	}
}

//=================================================================================================
void FlowContainer::UpdateText()
{
	batchChanges = false;

	const int shift = layout->border + layout->padding;
	int y = layout->border;
	bool haveButton = false;

	for(FlowItem* fi : items)
	{
		if(fi->type != FlowItem::Button)
		{
			if(fi->type != FlowItem::Section && haveButton)
				fi->pos = Int2(shift + 2 + buttonSize.x, y);
			else
				fi->pos = Int2(shift, y);
			haveButton = false;
			y += fi->size.y + 1;
		}
		else
		{
			fi->pos = Int2(shift, y);
			haveButton = true;
		}
	}

	scroll.total = y + layout->border - 1;
}
