#pragma once

//-----------------------------------------------------------------------------
enum class DialogType
{
	Ok,
	YesNo,
	YesNoCancel,
	Custom
};

//-----------------------------------------------------------------------------
enum class DialogOrder
{
	Normal,
	Top,
	TopMost
};

//-----------------------------------------------------------------------------
typedef delegate<void(int)> DialogEvent;
typedef delegate<void(int, int)> DialogEvent2;

//-----------------------------------------------------------------------------
struct DialogInfo
{
	DialogInfo() : custom_names(nullptr), img(nullptr), have_tick(false), ticked(false), auto_wrap(false), type(DialogType::Ok), parent(nullptr),
		order(DialogOrder::Normal), pause(true)
	{
	}

	string name, text;
	DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	cstring* custom_names, tick_text;
	Texture* img;
	bool pause, have_tick, ticked, auto_wrap;
};
