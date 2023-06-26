#include "Pch.h"
#include <cstdarg>
#include <sstream>
// for lstrlenW
#include "WindowsIncludes.h"

static const uint FORMAT_STRINGS = 8;
static const uint FORMAT_LENGTH = 2048;
static thread_local char format_buf[FORMAT_STRINGS][FORMAT_LENGTH];
static thread_local int format_marker;
static string g_escp;
string g_tmp_string;
static const char escape_from[] = { '\n', '\t', '\r', ' ' };
static cstring escape_to[] = { "\\n", "\\t", "\\r", " " };

//=================================================================================================
char* GetFormatString()
{
	char* str = format_buf[format_marker];
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return str;
}

//=================================================================================================
// Formatowanie ci¹gu znaków
//=================================================================================================
cstring Format(cstring str, ...)
{
	assert(str);

	va_list list;
	va_start(list, str);
	char* cbuf = GetFormatString();
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;
	va_end(list);

	return cbuf;
}

//=================================================================================================
cstring FormatList(cstring str, va_list list)
{
	assert(str);

	char* cbuf = GetFormatString();
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;

	return cbuf;
}

//=================================================================================================
void FormatStr(string& s, cstring str, ...)
{
	assert(str);
	va_list list;
	va_start(list, str);
	s.resize(FORMAT_LENGTH);
	char* cbuf = (char*)s.data();
	int len = _vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	if(len >= 0)
	{
		cbuf[len] = 0;
		s.resize(len);
	}
	else
	{
		cbuf[0] = 0;
		s.clear();
	}
	va_end(list);
}

//=================================================================================================
cstring Upper(cstring str)
{
	assert(str);

	char* cbuf = GetFormatString();
	if(*str == 0)
		cbuf[0] = 0;
	else
	{
		cbuf[0] = toupper(str[0]);
		strcpy_s(cbuf + 1, FORMAT_LENGTH, str + 1);
	}

	return cbuf;
}

//=================================================================================================
TextHelper::ParseResult TextHelper::ToNumber(cstring s, int64& i, float& f)
{
	assert(s);

	i = 0;
	f = 0;
	uint diver = 10;
	uint digits = 0;
	char c;
	bool sign = false;
	if(*s == '-')
	{
		sign = true;
		++s;
	}

	// parse integer part
	while((c = *s) != 0)
	{
		if(c == '.')
		{
			++s;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			i *= 10;
			i += (int)c - '0';
		}
		else
			return Broken;
		++s;
	}

	if(i > std::numeric_limits<uint>::max())
		return Broken;

	// end of string, this is int
	if(c == 0)
	{
		if(sign)
			i = -i;
		f = (float)i;
		return Int;
	}

	// parse fraction part
	f = (float)i;
	while((c = *s) != 0)
	{
		if(c == 'f')
		{
			if(digits == 0)
				return Broken;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			++digits;
			f += ((float)((int)c - '0')) / diver;
			diver *= 10;
		}
		else
			return Broken;
		++s;
	}

	if(sign)
	{
		i = -i;
		f = -f;
	}
	return Float;
}

//=================================================================================================
bool TextHelper::ToInt(cstring s, int& result)
{
	int64 i;
	float f;
	if(ToNumber(s, i, f) != Broken && InRange<int>(i))
	{
		result = (int)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToUint(cstring s, uint& result)
{
	int64 i;
	float f;
	if(ToNumber(s, i, f) != Broken)
	{
		result = (uint)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToFloat(cstring s, float& result)
{
	int64 i;
	float f;
	if(ToNumber(s, i, f) != Broken)
	{
		result = f;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool TextHelper::ToBool(cstring s, bool& result)
{
	if(_stricmp(s, "true") == 0)
	{
		result = true;
		return true;
	}
	else if(_stricmp(s, "false") == 0)
	{
		result = false;
		return true;
	}
	else
	{
		int value;
		if(!ToInt(s, value) && value != 0 && value != 1)
			return false;
		result = (value == 1);
		return true;
	}
}

//=================================================================================================
vector<string> Split(cstring str, const char delimiter, const char quote)
{
	assert(str);

	vector<string> results;

	while(*str)
	{
		while(*str == delimiter)
			++str;

		char c = *str;
		if(c)
		{
			cstring start;
			if(c == quote)
			{
				++str;
				start = str;
				while(*str && *str != quote)
					++str;
			}
			else
			{
				start = str;
				while(*str && *str != delimiter)
					++str;
			}

			results.push_back(string(start, str - start));
		}

		if(*str)
			++str;
	}

	return results;
}

//=================================================================================================
void SplitText(char* buf, vector<cstring>& lines)
{
	cstring start = buf;
	int len = 0;

	while(true)
	{
		char c = *buf;
		if(c == 0x0D || c == 0x0A)
		{
			if(len)
			{
				lines.push_back(start);
				len = 0;
			}
			start = buf + 1;
			*buf = 0;
		}
		else if(c == 0)
		{
			if(len)
				lines.push_back(start);
			break;
		}
		else
			++len;
		++buf;
	}
}

//=================================================================================================
bool Unescape(const string& str_in, uint pos, uint size, string& str_out)
{
	str_out.clear();
	str_out.reserve(str_in.length());

	cstring unesc = "nt\\\"'";
	cstring esc = "\n\t\\\"'";
	uint end = pos + size;

	for(; pos < end; ++pos)
	{
		if(str_in[pos] == '\\')
		{
			++pos;
			if(pos == end)
			{
				Error("Unescape error in string \"%.*s\", character '\\' at end of string.", size, str_in.c_str() + pos);
				return false;
			}
			int index = StrCharIndex(unesc, str_in[pos]);
			if(index != -1)
				str_out += esc[index];
			else
			{
				Error("Unescape error in string \"%.*s\", unknown escape sequence '\\%c'.", size, str_in.c_str() + pos, str_in[pos]);
				return false;
			}
		}
		else
			str_out += str_in[pos];
	}

	return true;
}

//=================================================================================================
cstring Escape(Cstring s, char quote)
{
	cstring str = s.s;
	char* out = GetFormatString();
	char* out_buf = out;
	cstring from = "\n\t\r\\";
	cstring to = "ntr\\";

	char c;
	while((c = *str) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				*out++ = '\\';
			*out++ = c;
		}
		else
		{
			*out++ = '\\';
			*out++ = to[index];
		}
		++str;
	}

	*out = 0;
	out_buf[FORMAT_LENGTH - 1] = 0;
	return out_buf;
}

//=================================================================================================
cstring Escape(Cstring str, string& out, char quote)
{
	cstring s = str.s;
	out.clear();
	cstring from = "\n\t\r";
	cstring to = "ntr";

	char c;
	while((c = *s) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				out += '\\';
			out += c;
		}
		else
		{
			out += '\\';
			out += to[index];
		}
		++s;
	}

	return out.c_str();
}

//=================================================================================================
cstring EscapeChar(char c)
{
	char* out = GetFormatString();
	for(uint i = 0; i < countof(escape_from); ++i)
	{
		if(c == escape_from[i])
		{
			strcpy_s(out, FORMAT_LENGTH, escape_to[i]);
			return out;
		}
	}

	if(isprint(c))
	{
		out[0] = c;
		out[1] = 0;
	}
	else
		_snprintf_s(out, FORMAT_LENGTH, FORMAT_LENGTH - 1, "0x%u", (uint)c);

	return out;
}

//=================================================================================================
cstring EscapeChar(char c, string& out)
{
	cstring esc = EscapeChar(c);
	out = esc;
	return out.c_str();
}

//=================================================================================================
bool StringInString(cstring s1, cstring s2)
{
	while(true)
	{
		if(*s1 == *s2)
		{
			++s1;
			++s2;
			if(*s2 == 0)
				return true;
		}
		else
			return false;
	}
}

//=================================================================================================
bool StringContainsStringI(cstring s1, cstring s2)
{
	while(true)
	{
		if(tolower(*s1) == tolower(*s2))
		{
			cstring sp1 = s1 + 1,
				sp2 = s2 + 1;
			while(true)
			{
				if(tolower(*sp1) == tolower(*sp2))
				{
					++sp1;
					++sp2;
					if(*sp2 == 0)
						return true;
				}
				else
					break;
			}
		}
		++s1;
		if(*s1 == 0)
			return false;
	}
}

//=================================================================================================
int StrCharIndex(cstring chrs, char c)
{
	int index = 0;
	do
	{
		if(*chrs == c)
			return index;
		++index;
		++chrs;
	}
	while(*chrs);

	return -1;
}

//=================================================================================================
char StrContains(cstring s, cstring chrs)
{
	assert(s && chrs);

	while(true)
	{
		char c = *s++;
		if(c == 0)
			return 0;
		cstring ch = chrs;
		while(true)
		{
			char c2 = *ch++;
			if(c2 == 0)
				break;
			if(c == c2)
				return c;
		}
	}
}

//=================================================================================================
char CharInStr(char c, cstring chrs)
{
	assert(chrs);

	while(true)
	{
		char c2 = *chrs++;
		if(c2 == 0)
			return 0;
		if(c == c2)
			return c;
	}
}

//=================================================================================================
cstring ToString(const wchar_t* wstr)
{
	assert(wstr);
	char* str = GetFormatString();
	size_t len;
	wcstombs_s(&len, str, FORMAT_LENGTH, wstr, FORMAT_LENGTH - 1);
	return str;
}

//=================================================================================================
const wchar_t* ToWString(cstring str)
{
	assert(str);
	size_t len;
	wchar_t* wstr = (wchar_t*)GetFormatString();
	mbstowcs_s(&len, wstr, FORMAT_LENGTH / 2, str, (FORMAT_LENGTH - 1) / 2);
	return wstr;
}

//=================================================================================================
void RemoveEndOfLine(string& str, bool remove)
{
	if(remove)
	{
		uint pos = 0;
		while(pos < str.length())
		{
			char c = str[pos];
			if(c == '\n' || c == '\r')
				str.erase(pos, 1);
			else
				++pos;
		}
	}
	else
	{
		uint pos = 0;
		while(pos < str.length())
		{
			char c = str[pos];
			if(c == '\r')
			{
				if(pos + 1 < str.length() && str[pos + 1] == '\n')
					str.erase(pos, 1);
				else
					str[pos] = '\n';
				++pos;
			}
			else
				++pos;
		}
	}
}

//=================================================================================================
void Replace(string& s, cstring in_chars, cstring out_chars)
{
	assert(in_chars && out_chars && strlen(in_chars) == strlen(out_chars));

	for(char& c : s)
	{
		cstring i_in_chars = in_chars,
			i_out_chars = out_chars;
		char i_char;
		while((i_char = *i_in_chars) != 0)
		{
			if(c == i_char)
				c = *i_out_chars;
			++i_in_chars;
			++i_out_chars;
		}
	}
}

//=================================================================================================
void MakeDoubleZeroTerminated(char* dest, Cstring src)
{
	cstring s = src.s;
	char c;
	while((c = *s++) != 0)
		*dest++ = c;
	*dest++ = 0;
	*dest = 0;
}

//=================================================================================================
uint FindClosingPos(const string& str, uint pos, char start, char end)
{
	assert(str[pos] == start);
	uint count = 1, len = str.length();
	++pos;
	while(true)
	{
		if(pos == len)
			return string::npos; // missing closing char
		char c = str[pos];
		if(c == start)
			++count;
		else if(c == end)
		{
			--count;
			if(count == 0)
				return pos;
		}
		++pos;
	}
}

//=================================================================================================
string UrlEncode(const string& s)
{
	static const char lookup[] = "0123456789abcdef";
	std::stringstream e;
	for(int i = 0, ix = s.length(); i < ix; i++)
	{
		const char& c = s[i];
		if((48 <= c && c <= 57) ||//0-9
			(65 <= c && c <= 90) ||//abc...xyz
			(97 <= c && c <= 122) || //ABC...XYZ
			(c == '-' || c == '_' || c == '.' || c == '~'))
		{
			e << c;
		}
		else
		{
			e << '%';
			e << lookup[(c & 0xF0) >> 4];
			e << lookup[(c & 0x0F)];
		}
	}
	return e.str();
}

//=================================================================================================
bool StartsWith(cstring str, cstring start)
{
	while(true)
	{
		char c = *start;
		if(c == 0)
			return true;
		if(c != *str)
			return false;
		++str;
		++start;
	}
}

//=================================================================================================
cstring ReplaceAll(cstring str, cstring from, cstring to)
{
	assert(str && from && to);

	cstring start = strstr(str, from);
	if(!start)
		return str;

	const int fromLen = strlen(from);

	char* cbuf = GetFormatString();
	char* out = cbuf;
	int count = start - str;
	for(int i = 0; i < count; ++i)
		*out++ = *str++;

	char c2;
	while((c2 = *str) != 0)
	{
		if(StartsWith(str, from))
		{
			cstring s = to;
			char c;
			while((c = *s) != 0)
			{
				*out++ = c;
				++s;
			}
			str += fromLen;
		}
		else
		{
			*out++ = c2;
			++str;
		}
	}

	*out = 0;
	return cbuf;
}

//=================================================================================================
cstring FindLastOf(cstring str, cstring chars)
{
	assert(str && chars);
	size_t len = strlen(str);
	cstring s = str + len - 1;
	while(s != str)
	{
		char c = *s;
		char c2;
		cstring ch = chars;
		while((c2 = *ch++) != 0)
		{
			if(c == c2)
				return s;
		}
		--s;
	}
	return s;
}

//=================================================================================================
// Simple converter utf8 -> Windows-1250
void Utf8ToAscii(string& str)
{
	byte* input = (byte*)(str.c_str());
	byte* output = input;
	const byte* end = input + str.length();
	while(input != end)
	{
		const byte b = *input++;
		if(b >= 128)
		{
			const byte b2 = *input++;
			const word code = (((word)b) << 8) | b2;
			byte r;
			switch(code)
			{
			case 0xC484:
				r = '¥';
				break;
			case 0xC486:
				r = 'Æ';
				break;
			case 0xC498:
				r = 'Ê';
				break;
			case 0xC581:
				r = '£';
				break;
			case 0xC583:
				r = 'Ñ';
				break;
			case 0xC393:
				r = 'Ó';
				break;
			case 0xC59A:
				r = 'Œ';
				break;
			case 0xC5B9:
				r = '';
				break;
			case 0xC5BB:
				r = '¯';
				break;
			case 0xC485:
				r = '¹';
				break;
			case 0xC487:
				r = 'æ';
				break;
			case 0xC499:
				r = 'ê';
				break;
			case 0xC582:
				r = '³';
				break;
			case 0xC584:
				r = 'ñ';
				break;
			case 0xC3B3:
				r = 'ó';
				break;
			case 0xC59B:
				r = 'œ';
				break;
			case 0xC5BA:
				r = 'Ÿ';
				break;
			case 0xC5BC:
				r = '¿';
				break;
			default:
				r = '?';
				break;
			}
			*output++ = r;
		}
		else
			*output++ = b;
	}

	str.resize(str.length() - (end - output));
}

//=================================================================================================
bool IsIdentifier(Cstring str)
{
	cstring s = str;
	char c;
	bool ok = false;
	while((c = *s++) != 0)
	{
		if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
			ok = true;
		else if(c >= '0' && c <= '9')
		{
			if(!ok)
				return false;
		}
		else
			return false;
	}
	return ok;
}
