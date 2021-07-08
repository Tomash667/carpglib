#pragma once

//-----------------------------------------------------------------------------
enum class Key : byte
{
	None = 0x00,
	LeftButton = 0x01,
	RightButton = 0x02,
	MiddleButton = 0x04,
	X1Button = 0x05,
	X2Button = 0x06,
	Backspace = 0x08,
	Tab = 0x09,
	Enter = 0x0D,
	Shift = 0x10,
	Control = 0x11,
	Alt = 0x12,
	Pause = 0x13,
	CapsLock = 0x14,
	Escape = 0x1B,
	Spacebar = 0x20,
	PageUp = 0x21,
	PageDown = 0x22,
	End = 0x23,
	Home = 0x24,
	Left = 0x25,
	Up = 0x26,
	Right = 0x27,
	Down = 0x28,
	PrintScreen = 0x2C,
	Insert = 0x2D,
	Delete = 0x2E,
	N1 = '1',
	N2 = '2',
	N3 = '3',
	N4 = '4',
	N5 = '5',
	N6 = '6',
	N7 = '7',
	N8 = '8',
	N9 = '9',
	N0 = '0',
	A = 'A',
	B = 'B',
	C = 'C',
	D = 'D',
	E = 'E',
	F = 'F',
	G = 'G',
	H = 'H',
	I = 'I',
	J = 'J',
	K = 'K',
	L = 'L',
	M = 'M',
	N = 'N',
	O = 'O',
	P = 'P',
	Q = 'Q',
	R = 'R',
	S = 'S',
	T = 'T',
	U = 'U',
	V = 'V',
	W = 'W',
	X = 'X',
	Y = 'Y',
	Z = 'Z',
	WindowsLeft = 0x5B,
	WindowsRight = 0x5C,
	Num0 = 0x60,
	Num1 = 0x61,
	Num2 = 0x62,
	Num3 = 0x63,
	Num4 = 0x64,
	Num5 = 0x65,
	Num6 = 0x66,
	Num7 = 0x67,
	Num8 = 0x68,
	Num9 = 0x69,
	NumMultiply = 0x6A,
	NumAdd = 0x6B,
	NumSubtract = 0x6D,
	NumDecimal = 0x6E,
	NumDivide = 0x6F,
	F1 = 0x70,
	F2 = 0x71,
	F3 = 0x72,
	F4 = 0x73,
	F5 = 0x74,
	F6 = 0x75,
	F7 = 0x76,
	F8 = 0x77,
	F9 = 0x78,
	F10 = 0x79,
	F11 = 0x7A,
	F12 = 0x7B,
	NumLock = 0x90,
	ScrollLock = 0x91,
	LeftShift = 0xA0,
	RightShift = 0xA1,
	LeftControl = 0xA2,
	RightControl = 0xA3,
	LeftAlt = 0xA4,
	RightAlt = 0xA5,
	Semicolon = 0xBA, // ;:
	Plus = 0xBB, // =+
	Comma = 0xBC, // ,<
	Minus = 0xBD, // -_
	Period = 0xBE, // .>
	Slash = 0xBF, // /?
	Tilde = 0xC0, // ~
	OpenSquareBracket = 0xDB, // [{
	Backslash = 0xDC, // \\|
	CloseSquareBracket = 0xDD, // ]}
	Quote = 0xDE, // '"

	Max = 0xFF
};

//-----------------------------------------------------------------------------
enum ShortcutKey
{
	KEY_SHIFT = 1 << 0,
	KEY_CONTROL = 1 << 1,
	KEY_ALT = 1 << 2
};

//-----------------------------------------------------------------------------
typedef array<Key, 2> KeyPair;

//-----------------------------------------------------------------------------
inline Key operator + (Key k, int offset)
{
	return (Key)((int)k + offset);
}
