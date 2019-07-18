#pragma once

#include "Key.h"

//-----------------------------------------------------------------------------
// stan klawisza
enum InputState
{
	IS_UP,			// 00
	IS_RELEASED,	// 01
	IS_DOWN,		// 10
	IS_PRESSED		// 11
};

enum ShortcutKey
{
	KEY_SHIFT = 1 << 0,
	KEY_CONTROL = 1 << 1,
	KEY_ALT = 1 << 2
};

//-----------------------------------------------------------------------------
// stan klawiatury
class Input
{
public:
	typedef bool (Input::*Func)(Key);
	typedef bool (Input::*FuncC)(Key) const;
	typedef delegate<void(Key)> Callback;
	static const uint MAX_KEY = (uint)Key::Max;

	Input();
	~Input();

	// proste sprawdzanie czy klawisz zosta³ wciœniêty, wyciœniêty, jest wciœniêty, jest wyciœniêty
	InputState GetState(Key key) const { return (InputState)keystate[(int)key]; }
	bool Pressed(Key key) const { return GetState(key) == IS_PRESSED; }
	bool Released(Key key) const { return GetState(key) == IS_RELEASED; }
	bool Down(Key key) const { return GetState(key) >= IS_DOWN; }
	bool Up(Key key) const { return GetState(key) <= IS_RELEASED; }

	// jednorazowe sprawdzanie czy klawisz jest wciœniêty, jeœli by³ to go ustawia na wyciœniêtego
	bool PressedRelease(Key key)
	{
		if(Pressed(key))
		{
			SetState(key, IS_DOWN);
			return true;
		}
		else
			return false;
	}

	bool PressedUp(Key key)
	{
		if(Pressed(key))
		{
			SetState(key, IS_UP);
			return true;
		}
		else
			return false;
	}

	// sprawdza czy jeden z dwóch klawiszy zosta³ wciœniêty
	Key Pressed2(Key k1, Key k2) const { return ReturnState2(k1, k2, IS_PRESSED); }
	// jw ale ustawia na wyciœniêty
	Key Pressed2Release(Key k1, Key k2)
	{
		if(GetState(k1) == IS_PRESSED)
		{
			SetState(k1, IS_DOWN);
			return k1;
		}
		else if(GetState(k2) == IS_PRESSED)
		{
			SetState(k2, IS_DOWN);
			return k2;
		}
		else
			return Key::None;
	}

	// sprawdza czy zosta³a wprowadzona kombinacja klawiszy (np alt+f4)
	bool DownPressed(Key k1, Key k2) const { return ((Down(k1) && Pressed(k2)) || (Down(k2) && Pressed(k1))); }

	// zwraca który z podanych klawiszy ma taki stan
	Key ReturnState2(Key k1, Key k2, InputState state) const
	{
		if(GetState(k1) == state)
			return k1;
		else if(GetState(k2) == state)
			return k2;
		else
			return Key::None;
	}

	// ustawia stan klawisza
	void SetState(Key key, InputState istate) { keystate[(int)key] = (byte)istate; }

	void Update();
	void UpdateShortcuts();
	void ReleaseKeys();
	void Process(Key key, bool down);
	void ProcessDoubleClick(Key key);

	byte* GetKeystateData()
	{
		return keystate;
	}

	void SetFocus(bool f) { focus = f; }
	bool Focus() const { return focus; }

	// shortcut, checks if other modifiers are not down
	// for example: Ctrl+A, shift and alt must not be pressed
	bool Shortcut(int modifier, Key key, bool up = true)
	{
		if(shortcut_state == modifier && Pressed(key))
		{
			if(up)
				SetState(key, IS_DOWN);
			return true;
		}
		else
			return false;
	}

	bool DownUp(Key key)
	{
		if(Down(key))
		{
			SetState(key, IS_UP);
			return true;
		}
		else
			return false;
	}

	bool DownRepeat(Key key)
	{
		return Down(key) && keyrepeat[(int)key];
	}

	bool DoubleClick(Key key)
	{
		assert(key >= Key::LeftButton && key <= Key::X2Button);
		return doubleclk[(int)key];
	}

	float GetMouseWheel() const { return mouse_wheel; }
	const Int2& GetMouseDif() const { return mouse_dif; }
	void UpdateMouseWheel(float mouse_wheel) { this->mouse_wheel = mouse_wheel; }
	void UpdateMouseDif(const Int2& mouse_dif) { this->mouse_dif = mouse_dif; }
	void SetCallback(Callback clbk) { key_callback = clbk; }

private:
	Callback key_callback;
	vector<Key> to_release;
	byte keystate[MAX_KEY];
	bool keyrepeat[MAX_KEY];
	bool doubleclk[5];
	Int2 mouse_dif;
	float mouse_wheel;
	int shortcut_state;
	bool focus;
};
