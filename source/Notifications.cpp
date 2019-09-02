#include "EnginePch.h"
#include "EngineCore.h"
#include "Notifications.h"
#include "Input.h"

static const Int2 notification_size(350, 80);

//=================================================================================================
void Notification::Close(float t)
{
	assert(t >= 0.f);
	if(t == 0.f)
	{
		if(state == State::Showing)
			t2 = 1.f - t2;
		state = State::Hiding;
		this->t = 0.f;
	}
	else
		this->t = t;
	buttons = false;
}

//=================================================================================================
Notifications::Notifications() : active_notifications()
{
}

//=================================================================================================
Notifications::~Notifications()
{
	for(int i = 0; i < MAX_ACTIVE_NOTIFICATIONS; ++i)
		delete active_notifications[i];
	DeleteElements(pending_notifications);
}

//=================================================================================================
Notification* Notifications::Add(cstring text, Texture* icon, float timer)
{
	assert(text);

	Notification* n = new Notification;
	n->text = text;
	n->icon = icon;
	n->t = timer;
	pending_notifications.push_back(n);
	return n;
}

//=================================================================================================
void Notifications::Draw(ControlDrawData*)
{
	bool any = false;
	for(int i = 0; i < MAX_ACTIVE_NOTIFICATIONS; ++i)
	{
		if(active_notifications[i])
		{
			any = true;
			break;
		}
	}
	if(!any)
		return;

	AreaLayout box_layout = layout->box;
	int offset_y = 8;

	for(Notification* n : active_notifications)
	{
		if(!n)
			continue;

		Int2 text_size = layout->font->CalculateSize(n->text, notification_size.x - 80);
		Int2 box_size = Int2::Max(notification_size, text_size + Int2(0, 16));
		Int2 real_box_size = box_size;
		if(n->buttons)
			real_box_size.y += 40;

		const int alpha = int(255 * n->t2);
		Int2 offset(gui->wnd_size.x - real_box_size.x - 8, offset_y);
		Color tint(255, 255, 255, alpha);

		gui->DrawArea(Box2d::Create(offset, real_box_size), layout->box, nullptr, &tint);

		if(n->icon)
			gui->DrawSprite(n->icon, offset + Int2(8, 8), Color::Alpha(alpha));

		Rect rect = { offset.x + 8 + 64, offset.y + 8, offset.x + box_size.x - 8, offset.y + box_size.y - 8 };
		gui->DrawText(layout->font, n->text, DTF_CENTER | DTF_VCENTER, Color(0, 0, 0, alpha), rect, &rect);

		if(n->buttons)
		{
			int text_size2 = rect.SizeX();

			// accept button
			Color color = n->active == 1 ? Color(255, 255, 255, alpha) : Color(0, 0, 0, alpha);
			gui->DrawArea(Box2d::Create(Int2(rect.p1.x + 4, offset.y + real_box_size.y - 46), Int2(text_size2 / 2 - 8, 36)), layout->button, nullptr, &color);
			gui->DrawSprite(layout->accept, Int2(rect.p1.x + (text_size2 / 2 - 32) / 2, offset.y + real_box_size.y - 44), color);

			// cancel button
			color = n->active == 2 ? Color(255, 255, 255, alpha) : Color(0, 0, 0, alpha);
			gui->DrawArea(Box2d::Create(Int2(rect.p1.x + text_size2 / 2 + 4, offset.y + real_box_size.y - 46), Int2(text_size2 / 2 - 8, 36)),
				layout->button, nullptr, &color);
			gui->DrawSprite(layout->cancel, Int2(rect.p1.x + text_size2 / 2 + (text_size2 / 2 - 32) / 2, offset.y + real_box_size.y - 44), color);
		}

		offset_y += real_box_size.y + 4;
	}
}

//=================================================================================================
void Notifications::Update(float dt)
{
	// count free notifications
	int free_items = 0;
	for(Notification* n : active_notifications)
	{
		if(!n)
			++free_items;
	}

	// add pending notification to active
	if(free_items > 0 && !pending_notifications.empty())
	{
		LoopAndRemove(pending_notifications, [&free_items, this](Notification* new_notification)
		{
			if(free_items == 0)
				return false;
			for(Notification*& n : active_notifications)
			{
				if(!n)
				{
					n = new_notification;
					--free_items;
					return true;
				}
			}
			return false;
		});
	}

	// update active notifications
	int offset_y = 8;
	for(Notification*& n : active_notifications)
	{
		if(!n)
			continue;

		if(n->state == Notification::Showing)
		{
			n->t2 += 3.f * dt;
			if(n->t2 >= 1.f)
			{
				n->state = Notification::Shown;
				n->t2 = 1.f;
			}
		}
		else if(n->state == Notification::Shown)
		{
			if(n->t >= 0.f)
			{
				n->t -= dt;
				if(n->t <= 0.f)
					n->state = Notification::Hiding;
			}
		}
		else
		{
			n->t2 -= dt;
			if(n->t2 <= 0.f)
			{
				delete n;
				n = nullptr;
				continue;
			}
		}

		Int2 text_size = layout->font->CalculateSize(n->text, notification_size.x - 80);
		Int2 box_size = Int2::Max(notification_size, text_size + Int2(0, 16));
		Int2 real_box_size = box_size;
		if(n->buttons && gui->NeedCursor())
		{
			real_box_size.y += 40;

			Int2 offset(gui->wnd_size.x - real_box_size.x - 8, 8);
			Rect rect = { offset.x + 8 + 64, offset.y + 8, offset.x + box_size.x - 8, offset.y + box_size.y - 8 };

			int text_size2 = rect.SizeX();
			if(PointInRect(gui->cursor_pos, Int2(rect.p1.x + 4, offset.y + real_box_size.y - 46), Int2(text_size2 / 2 - 8, 36)))
				n->active = 1;
			else if(PointInRect(gui->cursor_pos, Int2(rect.p1.x + text_size2 / 2 + 4, offset.y + real_box_size.y - 46), Int2(text_size2 / 2 - 8, 36)))
				n->active = 2;
			else
				n->active = 0;

			if(n->active != 0 && !gui->HaveDialog() && input->Pressed(Key::LeftButton))
				n->callback(n->active == 1);
		}
		else
			n->active = 0;

		offset_y += real_box_size.y + 4;
	}
}
