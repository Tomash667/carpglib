#pragma once

//-----------------------------------------------------------------------------
#include "Layout.h"

//-----------------------------------------------------------------------------
namespace layout
{
	struct Notifications : public Control
	{
		AreaLayout box;
		AreaLayout button;
		Font* font;
		TexturePtr accept, cancel;
	};
}

//-----------------------------------------------------------------------------
struct Notification
{
	friend class Notifications;

private:
	enum State
	{
		Showing,
		Shown,
		Hiding
	};

	Notification() : t2(0), state(Showing), buttons(false), active(0) {}

	State state;
	float t, t2;
	int active;

public:
	void Close(float t = 0.f);

	string text;
	Texture* icon;
	delegate<void(bool)> callback;
	bool buttons;
};

//-----------------------------------------------------------------------------
class Notifications : public Control, public LayoutControl<layout::Notifications>
{
public:
	static const int MAX_ACTIVE_NOTIFICATIONS = 3;

	Notifications();
	~Notifications();
	Notification* Add(cstring text, Texture* icon, float timer);
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;

private:
	Notification* active_notifications[MAX_ACTIVE_NOTIFICATIONS];
	vector<Notification*> pending_notifications;
};
