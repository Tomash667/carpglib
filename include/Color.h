#pragma once

struct Color
{
	union
	{
		uint value;
		struct
		{
			byte r;
			byte g;
			byte b;
			byte a;
		};
	};

	Color() {}
	constexpr Color(uint value) : value(value) {}
	constexpr Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}
	constexpr Color(float r, float g, float b, float a = 1.f) : r(byte(r * 255)), g(byte(g * 255)), b(byte(b * 255)), a(byte(a * 255)) {}
	constexpr Color(const Color& c) : value(c.value) {}

	bool operator == (Color c) const { return value == c.value; }
	bool operator != (Color c) const { return value != c.value; }

	Color operator * (Color c) const
	{
		return Color(r * c.r / 255, g * c.g / 255, b * c.b / 255, a * c.a / 255);
	}
	void operator *= (Color c)
	{
		r = byte(a * c.r / 255);
		g = byte(g * c.g / 255);
		b = byte(b * c.b / 255);
		a = byte(a * c.a / 255);
	}

	operator uint () const { return value; }
	operator Vec4 () const { return Vec4(float(r) / 255.f, float(g) / 255.f, float(b) / 255.f, float(a) / 255.f); }
	operator Vec3 () const { return Vec3(float(r) / 255.f, float(g) / 255.f, float(b) / 255.f); }

	static Color Lerp(Color c1, Color c2, float t)
	{
		return Color(::Lerp(c1.r, c2.r, t), ::Lerp(c1.g, c2.g, t), ::Lerp(c1.b, c2.b, t), ::Lerp(c1.a, c2.a, t));
	}
	static Color Alpha(int a)
	{
		return Color(255, 255, 255, a);
	}
	static Color Alpha(float a)
	{
		return Color(255, 255, 255, byte(a * 255));
	}
	static constexpr Vec4 Hex(uint h)
	{
		return Vec4(1.f / 256 * (((h) & 0xFF0000) >> 16), 1.f / 256 * (((h) & 0xFF00) >> 8), 1.f / 256 * ((h) & 0xFF), 1.f);
	}

	static const Color None;
	static const Color Black;
	static const Color Gray;
	static const Color White;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
};

inline constexpr const Color Color::None = Color(0, 0, 0, 0);
inline constexpr const Color Color::Black = Color(0, 0, 0);
inline constexpr const Color Color::Gray = Color(128, 128, 128);
inline constexpr const Color Color::White = Color(255, 255, 255);
inline constexpr const Color Color::Red = Color(255, 0, 0);
inline constexpr const Color Color::Green = Color(0, 255, 0);
inline constexpr const Color Color::Blue = Color(0, 0, 255);
inline constexpr const Color Color::Yellow = Color(255, 255, 0);
