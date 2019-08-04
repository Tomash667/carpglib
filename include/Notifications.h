#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Notifications : public Control
	{
		AreaLayout box;
		Font* font;
	};
}

//-----------------------------------------------------------------------------
class Notifications : public Control, public LayoutControl<layout::Notifications>
{
	struct Notification
	{
		enum State
		{
			Showing,
			Shown,
			Hiding
		};

		string text;
		Texture* icon;
		float t, t2;
		State state;
	};

	static const int MAX_ACTIVE_NOTIFICATIONS = 3;
	Notification* active_notifications[MAX_ACTIVE_NOTIFICATIONS];
	vector<Notification*> pending_notifications;

public:
	Notifications();
	~Notifications();
	void Add(cstring text, Texture* icon, float timer);
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;
};
