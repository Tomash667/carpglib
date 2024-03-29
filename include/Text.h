#pragma once

//-----------------------------------------------------------------------------
struct Cstring
{
	Cstring(cstring s) : s(s)
	{
		assert(s);
	}
	Cstring(const string& str) : s(str.c_str())
	{
	}
	Cstring(const LocalString& str) : s(str.c_str())
	{
	}

	operator cstring() const
	{
		return s;
	}

	cstring s;
};

inline bool operator == (const string& s1, const Cstring& s2)
{
	return s1 == s2.s;
}
inline bool operator == (const Cstring& s1, const string& s2)
{
	return s2 == s1.s;
}
inline bool operator == (cstring s1, const Cstring& s2)
{
	return strcmp(s1, s2.s) == 0;
}
inline bool operator == (const Cstring& s1, cstring s2)
{
	return strcmp(s1.s, s2) == 0;
}
inline bool operator != (const string& s1, const Cstring& s2)
{
	return s1 != s2.s;
}
inline bool operator != (const Cstring& s1, const string& s2)
{
	return s2 != s1.s;
}
inline bool operator != (cstring s1, const Cstring& s2)
{
	return strcmp(s1, s2.s) != 0;
}
inline bool operator != (const Cstring& s1, cstring s2)
{
	return strcmp(s1.s, s2) != 0;
}

//-----------------------------------------------------------------------------
struct CstringComparer
{
	bool operator() (cstring s1, cstring s2) const
	{
		return _stricmp(s1, s2) > 0;
	}
};
struct CstringEqualComparer
{
	bool operator() (cstring s1, cstring s2) const
	{
		return strcmp(s1, s2) == 0;
	}
};

//-----------------------------------------------------------------------------
char* GetFormatString();
cstring Format(cstring fmt, ...);
cstring FormatList(cstring fmt, va_list lis);
void FormatStr(string& str, cstring fmt, ...);
cstring Upper(cstring str);
vector<string> Split(cstring str, char delimiter = ' ', char quote = '"');
void SplitText(char* buf, vector<cstring>& lines);
bool Unescape(const string& strIn, uint pos, uint length, string& strOut);
inline bool Unescape(const string& strIn, string& strOut)
{
	return Unescape(strIn, 0u, strIn.length(), strOut);
}
bool StringInString(cstring s1, cstring s2);
cstring Escape(Cstring str, char quote = '"');
cstring Escape(Cstring str, string& out, char quote = '"');
cstring EscapeChar(char c);
cstring EscapeChar(char c, string& out);
cstring ToString(const wchar_t* str);
const wchar_t* ToWString(cstring str);
void Replace(string& str, cstring inChars, cstring outChars);
inline bool StartsWith(const string& value, const string& start)
{
	if(value.size() < start.size())
		return false;
	return std::equal(start.begin(), start.end(), value.begin());
}
inline bool EndsWith(const string& value, const string& ending)
{
	if(ending.size() > value.size())
		return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
void MakeDoubleZeroTerminated(char* dest, Cstring src);
bool StringContainsStringI(cstring s1, cstring s2);
// return index of character in cstring
int StrCharIndex(cstring chrs, char c);
char StrContains(cstring s, cstring chrs);
char CharInStr(char c, cstring chrs);
// convert \r \r\n -> \n or remove all
void RemoveEndOfLine(string& str, bool remove);
uint FindClosingPos(const string& str, uint pos, char start = '(', char end = ')');
string UrlEncode(const string& s);
bool StartsWith(cstring str, cstring start);
// replace all occurrences of 'from' with 'to'
cstring ReplaceAll(cstring str, cstring from, cstring to);
cstring FindLastOf(cstring str, cstring chars);
void Utf8ToAscii(string& str);
bool IsIdentifier(Cstring str);
int FindCharInString(cstring str, cstring chars);

namespace TextHelper
{
	enum ParseResult
	{
		Broken,
		Int,
		Float
	};

	ParseResult ToNumber(cstring s, int64& i, float& f);
	bool ToInt(cstring s, int& result);
	bool ToUint(cstring s, uint& result);
	bool ToFloat(cstring s, float& result);
	bool ToBool(cstring s, bool& result);
	bool IsNumber(cstring s);
}

// trim from start
inline string& Ltrim(string& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(), [](char& ch)->bool { return !isspace((byte)ch); }));
	return str;
}

// trim from end
inline string& Rtrim(string& str)
{
	str.erase(find_if(str.rbegin(), str.rend(), [](char& ch)->bool { return !isspace((byte)ch); }).base(), str.end());
	return str;
}

// trim from both ends
inline string& Trim(string& str)
{
	return Ltrim(Rtrim(str));
}

inline string Trimmed(const string& str)
{
	string s = str;
	Trim(s);
	return s;
}

inline string& Truncate(string& str, uint length)
{
	if(str.length() > length)
		str = str.substr(0, length);
	return str;
}
inline string Truncate(const string& str, uint length)
{
	if(str.length() > length)
		return str.substr(0, length);
	else
		return str;
}

template<typename T, class Pred>
inline void Join(const vector<T>& v, string& s, cstring separator, Pred pred)
{
	if(v.empty())
		return;
	if(v.size() == 1)
		s += pred(v.front());
	else
	{
		for(typename vector<T>::const_iterator it = v.begin(), end = v.end() - 1; it != end; ++it)
		{
			s += pred(*it);
			s += separator;
		}
		s += pred(*(v.end() - 1));
	}
}

template<int N>
inline cstring RandomString(cstring(&strs)[N])
{
	return strs[Rand() % N];
}
