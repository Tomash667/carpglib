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
	DialogInfo() : customNames(nullptr), img(nullptr), haveTick(false), ticked(false), autoWrap(false), type(DialogType::Ok), parent(nullptr),
		order(DialogOrder::Normal), pause(true)
	{
	}

	string name, text;
	DialogType type;
	Control* parent;
	DialogEvent event;
	DialogOrder order;
	cstring* customNames, tickText;
	Texture* img;
	bool pause, haveTick, ticked, autoWrap;
};
