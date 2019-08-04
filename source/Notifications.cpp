#include "EnginePch.h"
#include "EngineCore.h"
#include "Notifications.h"

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
void Notifications::Add(cstring text, Texture* icon, float timer)
{
	assert(text && timer > 0);

	Notification* n = new Notification;
	n->text = text;
	n->icon = icon;
	n->state = Notification::Showing;
	n->t = timer;
	n->t2 = 0.f;
	pending_notifications.push_back(n);
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

	static const Int2 notification_size(350, 80);
	AreaLayout box_layout = layout->box;

	for(Notification* n : active_notifications)
	{
		if(!n)
			continue;

		Int2 text_size = layout->font->CalculateSize(n->text, notification_size.x - 80);
		Int2 box_size = Int2::Max(notification_size, text_size + Int2(0, 16));

		const int alpha = int(255 * n->t2);
		Int2 offset(gui->wnd_size.x - box_size.x - 8, 8);
		box_layout.color.a = alpha;

		gui->DrawArea(Box2d::Create(offset, box_size), box_layout);

		if(n->icon)
			gui->DrawSprite(n->icon, offset + Int2(8, 8), Color::Alpha(alpha));

		Rect rect = { offset.x + 8 + 64, offset.y + 8, offset.x + box_size.x - 8, offset.y + box_size.y - 8 };
		gui->DrawText(layout->font, n->text, DTF_CENTER | DTF_VCENTER, Color(0, 0, 0, alpha), rect, &rect);
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
			n->t -= dt;
			if(n->t <= 0.f)
				n->state = Notification::Hiding;
		}
		else
		{
			n->t2 -= dt;
			if(n->t2 <= 0.f)
			{
				delete n;
				n = nullptr;
			}
		}
	}
}
