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
	Color(uint value) : value(value) {}
	Color(int r, int g, int b, int a = 255) : r(r), g(g), b(b), a(a) {}
	Color(float r, float g, float b, float a = 1.f) : r(byte(r * 255)), g(byte(g * 255)), b(byte(b * 255)), a(byte(a * 255)) {}
	Color(const Color& c) : value(c.value) {}

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
	static Color Alpha(Color c, int a)
	{
		return Color(c.r, c.g, c.b, a);
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
