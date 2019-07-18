#include "EnginePch.h"
#include "EngineCore.h"
#include "Input.h"

Input::Input() : mouse_wheel(0), mouse_dif(0, 0), key_callback(nullptr)
{
}

Input::~Input()
{
}

// change keys state from pressed->down and released->up
void Input::Update()
{
	byte printscreen = GetState(Key::PrintScreen);
	for(uint i = 0; i < MAX_KEY; ++i)
	{
		if(keystate[i] & 1)
			--keystate[i];
		keyrepeat[i] = false;
	}
	for(uint i = 0; i < 5; ++i)
		doubleclk[i] = false;
	if(printscreen == IS_PRESSED)
		SetState(Key::PrintScreen, IS_RELEASED);
	for(Key k : to_release)
		SetState(k, IS_RELEASED);
	to_release.clear();
}

void Input::UpdateShortcuts()
{
	shortcut_state = 0;
	if(Down(Key::Shift))
		shortcut_state |= KEY_SHIFT;
	if(Down(Key::Control))
		shortcut_state |= KEY_CONTROL;
	if(Down(Key::Alt))
		shortcut_state |= KEY_ALT;
}

// release all pressed/down keys
void Input::ReleaseKeys()
{
	for(uint i = 0; i < MAX_KEY; ++i)
	{
		if(keystate[i] & 0x2)
			keystate[i] = IS_RELEASED;
	}
	for(uint i = 0; i < 5; ++i)
		doubleclk[i] = false;
	to_release.clear();
}

// handle key down/up
void Input::Process(Key key, bool down)
{
	if(key_callback)
	{
		if(down)
			key_callback(key);
		return;
	}

	byte& k = keystate[(int)key];
	if(key != Key::PrintScreen)
	{
		if(down)
		{
			if(k <= IS_RELEASED)
				k = IS_PRESSED;
			keyrepeat[(int)key] = true;
		}
		else
		{
			if(k == IS_PRESSED)
				to_release.push_back(key);
			else if(k == IS_DOWN)
				k = IS_RELEASED;
		}
	}
	else
		k = IS_PRESSED;
}

void Input::ProcessDoubleClick(Key key)
{
	assert(key >= Key::LeftButton && key <= Key::X2Button);
	Process(key, true);
	if(!key_callback)
		doubleclk[(int)key] = true;
}
