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
		Pending,
		Showing,
		Shown,
		Hiding
	};

	Notification() : t2(0), buttons(false), active(0) {}

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
	Notifications();
	~Notifications();
	Notification* Add(cstring text, Texture* icon, float timer);
	uint GetMaxNotifications() const { return maxNotifications; }
	void SetShortcuts(KeyPair acceptKey, KeyPair declineKey, cstring acceptText, cstring declineText);
	void SetMaxNotifications(uint value) { assert(value >= 1u); maxNotifications = value; }
	void Draw() override;
	void Update(float dt) override;

private:
	vector<Notification*> notifications;
	uint maxNotifications;
	KeyPair acceptKey, declineKey;
	cstring acceptText, declineText;
};
