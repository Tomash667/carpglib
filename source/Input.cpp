#include "Pch.h"
#include "Input.h"

Input* app::input;

//=================================================================================================
Input::Input() : mouseWheel(0), mouseDif(0, 0), keyCallback(nullptr)
{
}

//=================================================================================================
Input::~Input()
{
}

//=================================================================================================
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
	if(printscreen == IS_PRESSED)
		SetState(Key::PrintScreen, IS_RELEASED);
	for(Key k : toRelease)
		SetState(k, IS_RELEASED);
	toRelease.clear();
}

//=================================================================================================
void Input::UpdateShortcuts()
{
	shortcutState = 0;
	if(Down(Key::Shift))
		shortcutState |= KEY_SHIFT;
	if(Down(Key::Control))
		shortcutState |= KEY_CONTROL;
	if(Down(Key::Alt))
		shortcutState |= KEY_ALT;
}

//=================================================================================================
// release all pressed/down keys
void Input::ReleaseKeys()
{
	for(uint i = 0; i < MAX_KEY; ++i)
	{
		if(keystate[i] & 0x2)
			keystate[i] = IS_RELEASED;
	}
	toRelease.clear();
}

//=================================================================================================
// handle key down/up
void Input::Process(Key key, bool down)
{
	if(keyCallback)
	{
		if(down)
			keyCallback(key);
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
				toRelease.push_back(key);
			else if(k == IS_DOWN)
				k = IS_RELEASED;
		}
	}
	else
		k = IS_PRESSED;
}
