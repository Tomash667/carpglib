#include "Pch.h"
#include "Font.h"

#include "DirectX.h"

//=================================================================================================
Font::Font() : tex(nullptr), texOutline(nullptr)
{
}

//=================================================================================================
Font::~Font()
{
	SafeRelease(tex);
	SafeRelease(texOutline);
}

//=================================================================================================
/* Taken from TFQ, modified for carpg
returns false when reached end of text
parametry:
outBegin - pierszy znak w tej linijce
outEnd - ostatni znak w tej linijce
outWidth - szerokoœæ tej linijki
inOutIndex - offset w text
text - tekst
textEnd - d³ugoœæ tekstu
flags - flagi (uwzglêdnia tylko DTF_SINGLELINE, DTF_PARSE_SPECIAL)
width - maksymalna szerokoœæ tej linijki
*/
bool Font::SplitLine(uint& outBegin, uint& outEnd, int& outWidth, uint& inOutIndex, cstring text, uint textEnd, uint flags, int width) const
{
	// Heh, piszê ten algorytm chyba trzeci raz w ¿yciu.
	// Za ka¿dym razem idzie mi szybciej i lepiej.
	// Ale zawsze i tak jest dla mnie ogromnie trudny i skomplikowany.
	if(inOutIndex >= textEnd)
		return false;

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	outBegin = inOutIndex;
	outWidth = 0;

	// Pojedyncza linia - specjalny szybki tryb
	if(IsSet(flags, DTF_SINGLELINE))
	{
		while(inOutIndex < textEnd)
		{
			char c = text[inOutIndex];
			if(c == '$' && parseSpecial)
			{
				if(SkipSpecial(inOutIndex, text, textEnd))
					continue;
			}
			else
				++inOutIndex;
			outWidth += GetCharWidth(c);
		}
		outEnd = inOutIndex;

		return true;
	}

	// Zapamiêtany stan miejsca ostatniego wyst¹pienia spacji
	// Przyda siê na wypadek zawijania wierszy na granicy s³owa.
	uint lastSpaceIndex = string::npos;
	int widthWhenLastSpace;

	for(;;)
	{
		// Koniec tekstu
		if(inOutIndex >= textEnd)
		{
			outEnd = textEnd;
			break;
		}

		// Pobierz znak
		char c = text[inOutIndex];

		// Koniec wiersza
		if(c == '\n')
		{
			outEnd = inOutIndex;
			inOutIndex++;
			break;
		}
		// Koniec wiersza \r
		else if(c == '\r')
		{
			outEnd = inOutIndex;
			inOutIndex++;
			// Sekwencja \r\n - pomiñ \n
			if(inOutIndex < textEnd && text[inOutIndex] == '\n')
				inOutIndex++;
			break;
		}
		else if(c == '$' && parseSpecial && SkipSpecial(inOutIndex, text, textEnd))
			continue;
		else
		{
			// Szerokoœæ znaku
			int charWidth = GetCharWidth(text[inOutIndex]);

			// Jeœli nie ma automatycznego zawijania wierszy lub
			// jeœli siê zmieœci lub
			// to jest pierwszy znak (zabezpieczenie przed nieskoñczonym zapêtleniem dla width < szerokoœæ pierwszego znaku) -
			// dolicz go i ju¿
			if(outWidth + charWidth <= width || inOutIndex == outBegin)
			{
				// Jeœli to spacja - zapamiêtaj dane
				if(c == ' ')
				{
					lastSpaceIndex = inOutIndex;
					widthWhenLastSpace = outWidth;
				}
				outWidth += charWidth;
				inOutIndex++;
			}
			// Jest automatyczne zawijanie wierszy i siê nie mieœci
			else
			{
				// Niemieszcz¹cy siê znak to spacja
				if(c == ' ')
				{
					outEnd = inOutIndex;
					// Mo¿na go przeskoczyæ
					inOutIndex++;
					break;
				}
				// Poprzedni znak za tym to spacja
				else if(inOutIndex > outBegin && text[inOutIndex - 1] == ' ')
				{
					// Koniec bêdzie na tej spacji
					outEnd = lastSpaceIndex;
					outWidth = widthWhenLastSpace;
					break;
				}

				// Zawijanie wierszy na granicy s³owa
				// By³a jakaœ spacja
				if(lastSpaceIndex != string::npos)
				{
					// Koniec bêdzie na tej spacji
					outEnd = lastSpaceIndex;
					inOutIndex = lastSpaceIndex + 1;
					outWidth = widthWhenLastSpace;
					break;
				}
				// Nie by³o spacji - trudno, zawinie siê jak na granicy znaku

				outEnd = inOutIndex;
				break;
			}
		}
	}

	return true;
}

//=================================================================================================
int Font::LineWidth(cstring str, bool parseSpecial) const
{
	int w = 0;

	while(true)
	{
		char c = *str;
		if(c == 0)
			break;

		if(c == '$' && parseSpecial)
		{
			++str;
			c = *str;
			assert(c);
			switch(c)
			{
			case '$':
				w += glyph[byte('$')].width;
				++str;
				break;
			case 'c':
				++str;
				++str;
				break;
			}

			continue;
		}

		w += glyph[byte(c)].width;
		++str;
	}

	return w;
}

//=================================================================================================
Int2 Font::CalculateSize(Cstring str, int limitWidth) const
{
	int len = strlen(str);
	cstring text = str;

	Int2 size(0, 0);

	uint unused, index = 0;
	int width;

	while(SplitLine(unused, unused, width, index, text, len, 0, limitWidth))
	{
		if(width > size.x)
			size.x = width;
		size.y += height;
	}

	return size;
}

//=================================================================================================
Int2 Font::CalculateSizeWrap(Cstring str, const Int2& maxSize, int border) const
{
	int maxWidth = maxSize.x - border;
	Int2 size = CalculateSize(str, maxWidth);
	int lines = size.y / height;
	int linePts = size.x / height;
	int totalPts = linePts * lines;

	while(line_pts > 15 + lines)
	{
		++lines;
		linePts = totalPts / lines;
	}

	return CalculateSize(str, linePts * height);
}

//=================================================================================================
bool Font::ParseGroupIndex(cstring text, uint lineEnd, uint& i, int& index, int& index2)
{
	index = -1;
	index2 = -1;
	LocalString tmpStr;
	bool first = true;
	while(true)
	{
		assert(i < lineEnd);
		char c = text[i];
		if(c >= '0' && c <= '9')
			tmpStr += c;
		else if(c == ',' && first && !tmpStr.empty())
		{
			first = false;
			index = atoi(tmpStr.c_str());
			tmpStr.clear();
		}
		else if(c == ';' && !tmpStr.empty())
		{
			int newIndex = atoi(tmpStr.c_str());
			if(first)
				index = newIndex;
			else
				index2 = newIndex;
			break;
		}
		else
		{
			// invalid hitbox counter
			assert(0);
			return false;
		}
		++i;
	}

	return true;
}

//=================================================================================================
bool Font::SkipSpecial(uint& inOutIndex, cstring text, uint textEnd) const
{
	// specjalna opcja
	inOutIndex++;
	if(inOutIndex < textEnd)
	{
		switch(text[inOutIndex])
		{
		case '$':
			return false;
		case 'c':
			// pomiñ kolor
			++inOutIndex;
			++inOutIndex;
			break;
		case 'h':
			// pomiñ hitbox
			++inOutIndex;
			++inOutIndex;
			break;
		case 'g':
			++inOutIndex;
			if(text[inOutIndex] == '+')
			{
				++inOutIndex;
				int tmp;
				ParseGroupIndex(text, textEnd, inOutIndex, tmp, tmp);
				++inOutIndex;
			}
			else if(text[inOutIndex] == '-')
				++inOutIndex;
			else
			{
				// unknown option
				assert(0);
				++inOutIndex;
			}
			break;
		default:
			// nieznana opcja
			++inOutIndex;
			assert(0);
			break;
		}
		return true;
	}
	else
	{
		// uszkodzony format tekstu, olej to
		++inOutIndex;
		assert(0);
		return true;
	}
}

//=================================================================================================
bool Font::HitTest(Cstring str, int limitWidth, int flags, const Int2& pos, uint& index, Int2& index2, Rect& rect, float& uv, const vector<Line>* fontLines) const
{
	if(pos.x < 0 || pos.y < 0)
		return false;

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	uint textEnd = strlen(str);
	cstring text = str;
	int width = 0, prevWidth = 0;
	index = 0;

	if(text_end == 0u)
	{
		index = 0;
		index2 = Int2::Zero;
		rect.p1 = Int2::Zero;
		rect.p2 = Int2(0, height);
		uv = 0;
		return true;
	}

	// simple single line mode
	if(IsSet(flags, DTF_SINGLELINE))
	{
		if(pos.y > height)
			return false;

		while(index < textEnd)
		{
			char c = text[index];
			if(c == '$' && parseSpecial)
			{
				if(SkipSpecial(index, text, textEnd))
					continue;
			}
			else
				++index;
			prevWidth = width;
			width += GetCharWidth(c);
			if(width >= pos.x)
				break;
		}
		--index;
		rect.p1 = Int2(prevWidth, 0);
		rect.p2 = Int2(width, height);
		uv = min(1.f, 1.f - float(width - pos.x) / (width - prevWidth));
		index2 = Int2(index, 0);
		return true;
	}

	// get correct line
	int line = pos.y / height;
	uint lineBegin, lineEnd;
	if(fontLines)
	{
		line = min(fontLines->size() - 1, (uint)line);
		auto& fontLine = fontLines->at(line);
		lineBegin = fontLine.begin;
		lineEnd = fontLine.end;
	}
	else
	{
		int currentLine = 0;
		do
		{
			if(!SplitLine(lineBegin, lineEnd, width, index, text, textEnd, flags, limitWidth))
			{
				line = currentLine - 1;
				break;
			}
			if(currentLine == line)
				break;
			++currentLine;
		} while(true);
	}

	index = lineBegin;
	width = 0;
	while(index < lineEnd)
	{
		char c = text[index];
		if(c == '$' && parseSpecial)
		{
			if(SkipSpecial(index, text, lineEnd))
				continue;
		}
		else
			++index;
		prevWidth = width;
		width += GetCharWidth(c);
		if(width >= pos.x)
			break;
	}
	if(index > 0)
		--index;
	rect.p1 = Int2(prevWidth, line * height);
	rect.p2 = Int2(width, (line + 1) * height);
	uv = min(1.f, 1.f - float(width - pos.x) / (width - prevWidth));
	index2 = Int2(index - lineBegin, line);
	return true;
}

//=================================================================================================
Int2 Font::IndexToPos(uint expectedIndex, Cstring str, int limitWidth, int flags) const
{
	assert(expectedIndex <= strlen(str));

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	uint textEnd = strlen(str);
	cstring text = str;
	uint index = 0;

	if(IsSet(flags, DTF_SINGLELINE))
	{
		int width = 0;

		while(index < textEnd && index != expectedIndex)
		{
			char c = text[index];
			if(c == '$' && parseSpecial)
			{
				if(SkipSpecial(index, text, textEnd))
					continue;
			}
			else
				++index;

			int charWidth = GetCharWidth(c);
			width += charWidth;
		}

		return Int2(width, 0);
	}

	uint lineBegin, lineEnd;
	uint line = 0;
	while(true)
	{
		int lineWidth;
		[[maybe_unused]] bool ok = SplitLine(lineBegin, lineEnd, lineWidth, index, text, textEnd, flags, limitWidth);
		assert(ok);
		if(expectedIndex >= lineBegin && expectedIndex <= lineEnd)
		{
			int width = 0;
			index = lineBegin;
			while(index < lineEnd && index != expectedIndex)
			{
				char c = text[index];
				if(c == '$' && parseSpecial)
				{
					if(SkipSpecial(index, text, textEnd))
						continue;
				}
				else
					++index;

				int charWidth = GetCharWidth(c);
				width += charWidth;
			}

			return Int2(width, line * height);
		}
		++line;
	}
}

//=================================================================================================
Int2 Font::IndexToPos(const Int2& expectedIndex, Cstring str, int limitWidth, int flags) const
{
	assert(expectedIndex.x >= 0 && expectedIndex.y >= 0);

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	uint textEnd = strlen(str);
	cstring text = str;
	uint index = 0;

	if(IsSet(flags, DTF_SINGLELINE))
	{
		assert(expectedIndex.x <= (int)textEnd);
		assert(expectedIndex.y == 0);

		int width = 0;

		while(index < textEnd && index != expectedIndex.x)
		{
			char c = text[index];
			if(c == '$' && parseSpecial)
			{
				if(SkipSpecial(index, text, textEnd))
					continue;
			}
			else
				++index;

			int charWidth = GetCharWidth(c);
			width += charWidth;
		}

		return Int2(width, 0);
	}

	uint lineBegin, lineEnd;
	uint line = 0;
	while(true)
	{
		int lineWidth;
		[[maybe_unused]] bool ok = SplitLine(lineBegin, lineEnd, lineWidth, index, text, textEnd, flags, limitWidth);
		assert(ok);
		if(line == expectedIndex.y)
		{
			assert((uint)expectedIndex.x < lineEnd - lineBegin);
			int width = 0;
			index = lineBegin;
			while(index < lineEnd && index - lineBegin != expectedIndex.x)
			{
				char c = text[index];
				if(c == '$' && parseSpecial)
				{
					if(SkipSpecial(index, text, textEnd))
						continue;
				}
				else
					++index;

				int charWidth = GetCharWidth(c);
				width += charWidth;
			}

			return Int2(width, line * height);
		}
		++line;
	}
}

//=================================================================================================
uint Font::PrecalculateFontLines(vector<Line>& fontLines, Cstring str, int limitWidth, int flags) const
{
	fontLines.clear();

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	uint textEnd = strlen(str);
	cstring text = str;
	uint index = 0;

	if(IsSet(flags, DTF_SINGLELINE))
	{
		uint width = GetLineWidth(text, 0, textEnd, parseSpecial);
		fontLines.push_back({ 0, textEnd, textEnd, width });
		return width;
	}
	else
	{
		uint lineBegin, lineEnd = 0, maxWidth = 0;
		int lineWidth;
		while(SplitLine(lineBegin, lineEnd, lineWidth, index, text, textEnd, flags, limitWidth))
		{
			fontLines.push_back({ lineBegin, lineEnd, lineEnd - lineBegin, (uint)lineWidth });
			if(lineWidth > (int)maxWidth)
				maxWidth = lineWidth;
		}
		if(fontLines.empty())
			fontLines.push_back({ 0, 0, 0, 0 });
		else if(fontLines.back().end != textEnd)
			fontLines.push_back({ textEnd, textEnd, 0, 0 });

		return maxWidth;
	}
}

//=================================================================================================
uint Font::GetLineWidth(cstring text, uint lineBegin, uint lineEnd, bool parseSpecial) const
{
	uint width = 0;
	uint index = lineBegin;

	while(index < lineEnd)
	{
		char c = text[index];
		if(c == '$' && parseSpecial)
		{
			if(SkipSpecial(index, text, lineEnd))
				continue;
		}
		else
			++index;

		int charWidth = GetCharWidth(c);
		width += charWidth;
	}

	return width;
}

//=================================================================================================
Int2 Font::IndexToPos(vector<Line>& fontLines, const Int2& expectedIndex, Cstring str, int limitWidth, int flags) const
{
	assert(expectedIndex.x >= 0 && expectedIndex.y >= 0);

	bool parseSpecial = IsSet(flags, DTF_PARSE_SPECIAL);
	uint textEnd = strlen(str);
	cstring text = str;
	uint index = 0;
	int width = 0;

	if(IsSet(flags, DTF_SINGLELINE))
	{
		assert(expectedIndex.x <= (int)textEnd);
		assert(expectedIndex.y == 0);

		while(index < textEnd && index != expectedIndex.x)
		{
			char c = text[index];
			if(c == '$' && parseSpecial)
			{
				if(SkipSpecial(index, text, textEnd))
					continue;
			}
			else
				++index;

			int charWidth = GetCharWidth(c);
			width += charWidth;
		}

		return Int2(width, 0);
	}

	assert((uint)expectedIndex.y < fontLines.size());
	auto& line = fontLines[expectedIndex.y];
	assert((uint)expectedIndex.x <= line.count);

	index = line.begin;
	while(index < line.end && index - line.begin != expectedIndex.x)
	{
		char c = text[index];
		if(c == '$' && parseSpecial)
		{
			if(SkipSpecial(index, text, textEnd))
				continue;
		}
		else
			++index;

		int charWidth = GetCharWidth(c);
		width += charWidth;
	}

	return Int2(width, expectedIndex.y * height);
}

//=================================================================================================
uint Font::ToRawIndex(vector<Line>& fontLines, const Int2& index) const
{
	assert(index.x >= 0 && index.y >= 0 && index.y < (int)fontLines.size());
	auto& line = fontLines[index.y];
	assert(index.x <= (int)line.count);
	return line.begin + index.x;
}

//=================================================================================================
Int2 Font::FromRawIndex(vector<Line>& fontLines, uint index) const
{
	uint lineIndex = 0;
	for(auto& line : fontLines)
	{
		if(index <= line.end)
			return Int2(index - line.begin, lineIndex);
		++lineIndex;
	}

	assert(0);
	return Int2(0, 0);
}
