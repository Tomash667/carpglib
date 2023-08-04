#pragma once

//-----------------------------------------------------------------------------
#include "Resource.h"

//-----------------------------------------------------------------------------
// Draw text flags
enum DrawTextFlag
{
	DTF_LEFT = 0,
	DTF_TOP = 0,
	DTF_CENTER = 1 << 0,
	DTF_RIGHT = 1 << 1,
	DTF_VCENTER = 1 << 2,
	DTF_BOTTOM = 1 << 3,
	DTF_SINGLELINE = 1 << 4,
	DTF_PARSE_SPECIAL = 1 << 5, // parse $ to set color/hitbox
	DTF_OUTLINE = 1 << 6, // draw outline around text
	DTF_DONT_DRAW = 1 << 7 // only calculate size
};

//-----------------------------------------------------------------------------
// Czcionka
struct Font : public Resource
{
	static const ResourceType Type = ResourceType::Font;

	struct Glyph
	{
		Box2d uv;
		int width;
	};

	struct Line
	{
		uint begin, end, count, width;
	};

	TEX tex, texOutline;
	int height, outline;
	Vec2 outlineShift;
	Glyph glyph[256];

	Font();
	~Font();
	// zwraca szerokoœæ znaku
	int GetCharWidth(char c) const { return glyph[byte(c)].width; }
	// oblicza szerokoœæ pojedyñczej linijki tekstu
	int LineWidth(cstring str, bool parseSpecial = false) const;
	// oblicza wysokoœæ i szerokoœæ bloku tekstu
	Int2 CalculateSize(Cstring str, int limitWidth = INT_MAX) const;
	Int2 CalculateSizeWrap(Cstring str, const Int2& maxSize, int border = 32) const;
	// split text between lines
	bool SplitLine(uint& outBegin, uint& outEnd, int& outWidth, uint& inOutIndex, cstring text, uint textEnd, uint flags, int width) const;

	static bool ParseGroupIndex(cstring text, uint lineEnd, uint& i, int& index, int& index2);
	bool HitTest(Cstring str, int limitWidth, int flags, const Int2& pos, uint& index, Int2& index2, Rect& rect, float& uv,
		const vector<Line>* fontLines = nullptr) const;

	// calculate position (top left corner of glyph) from index
	Int2 IndexToPos(uint index, Cstring str, int limitWidth, int flags) const;
	Int2 IndexToPos(const Int2& index, Cstring str, int limitWidth, int flags) const;
	Int2 IndexToPos(vector<Line>& fontLines, const Int2& index, Cstring str, int limitWidth, int flags) const;

	// precalculate line begin/end position, width, returns max width
	uint PrecalculateFontLines(vector<Line>& fontLines, Cstring str, int limitWidth, int flags) const;
	uint ToRawIndex(vector<Line>& fontLines, const Int2& index) const;
	Int2 FromRawIndex(vector<Line>& fontLines, uint index) const;

private:
	bool SkipSpecial(uint& inOutIndex, cstring text, uint textEnd) const;
	uint GetLineWidth(cstring text, uint lineBegin, uint lineEnd, bool parseSpecial) const;
};
