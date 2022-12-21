#include "Pch.h"
#include "Notifications.h"

#include "Input.h"

static const Int2 notificationSize(350, 80);

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
Notifications::Notifications() : maxNotifications(3), acceptKey({}), declineKey({}), acceptText(nullptr), declineText(nullptr)
{
}

//=================================================================================================
Notifications::~Notifications()
{
	DeleteElements(notifications);
}

//=================================================================================================
void Notifications::SetShortcuts(KeyPair acceptKey, KeyPair declineKey, cstring acceptText, cstring declineText)
{
	this->acceptKey = acceptKey;
	this->declineKey = declineKey;
	this->acceptText = acceptText;
	this->declineText = declineText;
}

//=================================================================================================
Notification* Notifications::Add(cstring text, Texture* icon, float timer)
{
	assert(text);

	Notification* n = new Notification;
	n->text = text;
	n->icon = icon;
	n->t = timer;
	n->state = (notifications.size() >= maxNotifications ? Notification::Pending : Notification::Showing);
	notifications.push_back(n);
	return n;
}

//=================================================================================================
void Notifications::Draw()
{
	if(notifications.empty())
		return;

	AreaLayout boxLayout = layout->box;
	int offetY = 8;
	bool firstWithButtons = true;

	for(Notification* n : notifications)
	{
		if(n->state == Notification::Pending)
			break;

		Int2 textSize = layout->font->CalculateSize(n->text, notificationSize.x - 80);
		Int2 boxSize = Int2::Max(notificationSize, textSize + Int2(0, 16));
		Int2 realBoxSize = boxSize;
		if(n->buttons)
			realBoxSize.y += 40;

		const int alpha = int(255 * n->t2);
		Int2 offset(gui->wndSize.x - realBoxSize.x - 8, offetY);
		Color tint(255, 255, 255, alpha);

		gui->DrawArea(Box2d::Create(offset, realBoxSize), layout->box, nullptr, &tint);

		if(n->icon)
			gui->DrawSprite(n->icon, offset + Int2(8, 8), Color::Alpha(alpha));

		Rect rect = { offset.x + 8 + 64, offset.y + 8, offset.x + boxSize.x - 8, offset.y + boxSize.y - 8 };
		gui->DrawText(layout->font, n->text, DTF_CENTER | DTF_VCENTER, Color(0, 0, 0, alpha), rect, &rect);

		if(n->buttons)
		{
			int textSize2 = rect.SizeX();

			// accept button
			Color color = n->active == 1 ? Color(255, 255, 255, alpha) : Color(0, 0, 0, alpha);
			gui->DrawArea(Box2d::Create(Int2(rect.p1.x + 4, offset.y + realBoxSize.y - 46), Int2(textSize2 / 2 - 8, 36)), layout->button, nullptr, &color);
			if(firstWithButtons && acceptText)
			{
				const int textWidth = layout->font->CalculateSize(acceptText).y;
				const int totalWidth = 32 + 8 + textWidth;
				gui->DrawSprite(layout->accept, Int2(rect.p1.x + (textSize2 / 2 - totalWidth) / 2, offset.y + realBoxSize.y - 44), color);
				gui->DrawText(layout->font, acceptText, DTF_LEFT | DTF_VCENTER, Color(0, 0, 0, alpha),
					Rect::Create(Int2(rect.p1.x + (textSize2 / 2 - totalWidth) / 2 + 40, offset.y + realBoxSize.y - 46), Int2(100, 36)));
			}
			else
				gui->DrawSprite(layout->accept, Int2(rect.p1.x + (textSize2 / 2 - 32) / 2, offset.y + realBoxSize.y - 44), color);

			// cancel button
			color = n->active == 2 ? Color(255, 255, 255, alpha) : Color(0, 0, 0, alpha);
			gui->DrawArea(Box2d::Create(Int2(rect.p1.x + textSize2 / 2 + 4, offset.y + realBoxSize.y - 46), Int2(textSize2 / 2 - 8, 36)),
				layout->button, nullptr, &color);
			if(firstWithButtons && declineText)
			{
				const int textWidth = layout->font->CalculateSize(declineText).y;
				const int totalWidth = 32 + 8 + textWidth;
				gui->DrawSprite(layout->cancel, Int2(rect.p1.x + textSize2 / 2 + (textSize2 / 2 - totalWidth) / 2, offset.y + realBoxSize.y - 44), color);
				gui->DrawText(layout->font, declineText, DTF_LEFT | DTF_VCENTER, Color(0, 0, 0, alpha),
					Rect::Create(Int2(rect.p1.x + textSize2 / 2 + (textSize2 / 2 - totalWidth) / 2 + 40, offset.y + realBoxSize.y - 46), Int2(100, 36)));
			}
			else
				gui->DrawSprite(layout->cancel, Int2(rect.p1.x + textSize2 / 2 + (textSize2 / 2 - 32) / 2, offset.y + realBoxSize.y - 44), color);

			firstWithButtons = false;
		}

		offetY += realBoxSize.y + 4;
	}
}

//=================================================================================================
void Notifications::Update(float dt)
{
	int offetY = 8;
	bool firstWithButtons = true;
	uint index = 0;
	LoopAndRemove(notifications, [&](Notification* n)
	{
		switch(n->state)
		{
		case Notification::Pending:
			if(index < maxNotifications)
				n->state = Notification::Showing;
			break;
		case Notification::Showing:
			n->t2 += 3.f * dt;
			if(n->t2 >= 1.f)
			{
				n->state = Notification::Shown;
				n->t2 = 1.f;
			}
			break;
		case Notification::Shown:
			if(n->t >= 0.f)
			{
				n->t -= dt;
				if(n->t <= 0.f)
					n->state = Notification::Hiding;
			}
			break;
		case Notification::Hiding:
			n->t2 -= dt;
			if(n->t2 <= 0.f)
			{
				delete n;
				return true;
			}
			break;
		}

		++index;

		Int2 textSize = layout->font->CalculateSize(n->text, notificationSize.x - 80);
		Int2 boxSize = Int2::Max(notificationSize, textSize + Int2(0, 16));
		Int2 realBoxSize = boxSize;
		if(n->buttons)
		{
			if(gui->NeedCursor())
			{
				realBoxSize.y += 40;

				Int2 offset(gui->wndSize.x - realBoxSize.x - 8, offetY);
				Rect rect = { offset.x + 8 + 64, offset.y + 8, offset.x + boxSize.x - 8, offset.y + boxSize.y - 8 };

				int textSize2 = rect.SizeX();
				if(Rect::IsInside(gui->cursorPos, Int2(rect.p1.x + 4, offset.y + realBoxSize.y - 46), Int2(textSize2 / 2 - 8, 36)))
					n->active = 1;
				else if(Rect::IsInside(gui->cursorPos, Int2(rect.p1.x + textSize2 / 2 + 4, offset.y + realBoxSize.y - 46), Int2(textSize2 / 2 - 8, 36)))
					n->active = 2;
				else
					n->active = 0;

				if(n->active != 0 && !gui->HaveDialog())
				{
					gui->SetCursorMode(CURSOR_HOVER);
					if(input->Pressed(Key::LeftButton))
					{
						n->callback(n->active == 1);
						firstWithButtons = false;
					}
				}
			}
			else
				n->active = 0;

			if(firstWithButtons)
			{
				firstWithButtons = false;
				if(input->Pressed(acceptKey[0]) || input->Pressed(acceptKey[1]))
					n->callback(true);
				else if(input->Pressed(declineKey[0]) || input->Pressed(declineKey[1]))
					n->callback(false);
			}
		}
		else
			n->active = 0;

		offetY += realBoxSize.y + 4;
		return false;
	});
}
