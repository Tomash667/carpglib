#include "Pch.h"
#include "Tokenizer.h"
#include "File.h"

using namespace tokenizer;

static cstring ALTER_START = "${";
static cstring ALTER_END = "}$";
static cstring WHITESPACE_SYMBOLS = " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\"";
static cstring WHITESPACE_SYMBOLS_DOT = " \t\n\r,/;'\\[]`<>?:|{}=~!@#$%^&*()+-\".";
static cstring SYMBOLS = ",./;'\\[]`<>?:|{}=~!@#$%^&*()+-";

//=================================================================================================
Tokenizer::Tokenizer(int _flags) : needSorting(false), formatter(this), seek(nullptr), ownString(false)
{
	SetFlags(_flags);
	Reset();
}

//=================================================================================================
Tokenizer::~Tokenizer()
{
	if(ownString)
		StringPool.SafeFree(const_cast<string*>(str));
	delete seek;
}

//=================================================================================================
void Tokenizer::FromString(cstring _str)
{
	assert(_str);
	if(!ownString)
		str = StringPool.Get();
	*const_cast<string*>(str) = _str;
	ownString = true;
	Reset();
}

//=================================================================================================
void Tokenizer::FromString(const string& _str)
{
	if(ownString)
	{
		StringPool.Free(const_cast<string*>(str));
		ownString = false;
	}
	str = &_str;
	Reset();
}

//=================================================================================================
bool Tokenizer::FromFile(cstring path)
{
	assert(path);
	if(!ownString)
	{
		str = StringPool.Get();
		ownString = true;
	}
	if(!io::LoadFileToString(path, *const_cast<string*>(str)))
		return false;
	filename = path;
	Reset();
	return true;
}

//=================================================================================================
void Tokenizer::FromTokenizer(const Tokenizer& t)
{
	if(ownString)
	{
		StringPool.Free(const_cast<string*>(str));
		ownString = false;
	}

	str = t.str;
	normalSeek.pos = t.normalSeek.pos;
	normalSeek.line = t.normalSeek.line;
	normalSeek.charpos = t.normalSeek.charpos;
	normalSeek.item = t.normalSeek.item;
	normalSeek.token = t.normalSeek.token;
	normalSeek._int = t.normalSeek._int;
	flags = t.flags;
	normalSeek._float = t.normalSeek._float;
	normalSeek._char = t.normalSeek._char;
	normalSeek._uint = t.normalSeek._uint;

	if(normalSeek.token == T_KEYWORD)
	{
		// need to check keyword because keywords are not copied from other tokenizer, it may be item here
		CheckItemOrKeyword(normalSeek, normalSeek.item);
	}
}

//=================================================================================================
bool Tokenizer::DoNext(SeekData& s, bool returnEol)
{
	CheckSorting();

redo:
	if(s.token == T_EOF)
		return false;

	if(s.pos >= str->length())
	{
		s.token = T_EOF;
		return false;
	}

	s.startPos = s.pos;

	// szukaj czegoœ
	uint pos2 = FindFirstNotOf(s, returnEol ? " \t" : " \t\n\r", s.pos);
	if(pos2 == string::npos)
	{
		// same spacje, entery, taby
		// to koniec pliku
		s.token = T_EOF;
		return false;
	}

	char c = str->at(pos2);

	if(c == '\r')
	{
		s.pos = pos2 + 1;
		if(s.pos < str->length() && str->at(s.pos) == '\n')
			++s.pos;
		s.token = T_EOL;
	}
	else if(c == '\n')
	{
		s.pos = pos2 + 1;
		s.token = T_EOL;
	}
	else if(c == '/')
	{
		char c2 = 0;
		if(pos2 + 1 != str->size())
			c2 = str->at(pos2 + 1);
		if(c2 == '/')
		{
			s.pos = FindFirstOf(s, "\n", pos2 + 1);
			if(s.pos == string::npos)
			{
				s.token = T_EOF;
				return false;
			}
			else
				goto redo;
		}
		else if(c2 == '*')
		{
			int prevLine = s.line;
			int prevCharpos = s.charpos;
			s.pos = FindFirstOfStr(s, "*/", pos2 + 1);
			if(s.pos == string::npos)
				formatter.Throw(Format("Not closed comment started at line %d, character %d.", prevLine + 1, prevCharpos + 1));
			goto redo;
		}
		else
		{
			++s.charpos;
			s.pos = pos2 + 1;
			s.token = T_SYMBOL;
			s._char = c;
			s.item = c;
		}
	}
	else if(c == '"')
	{
		// szukaj koñca ci¹gu znaków
		uint cp = s.charpos;
		s.pos = FindEndOfQuote(s, pos2 + 1);

		if(s.pos == string::npos || str->at(s.pos) != '"')
			formatter.Throw(Format("Not closed string \" opened at %u.", cp + 1));

		Unescape(*str, pos2 + 1, s.pos - pos2 - 1, s.item);
		s.token = T_STRING;
		++s.pos;
	}
	else if(StringInString(str->c_str() + pos2, ALTER_START))
	{
		// alter string
		int len = strlen(ALTER_START);
		s.pos = pos2 + len;
		s.charpos += len;
		uint blockStart = s.pos;
		bool ok = false;
		len = strlen(ALTER_END);

		for(; s.pos < str->length(); ++s.pos)
		{
			if(StringInString(str->c_str() + s.pos, ALTER_END))
			{
				s.item = str->substr(blockStart, s.pos - blockStart);
				s.token = T_STRING;
				s.pos += len;
				s.charpos += len;
				ok = true;
				break;
			}
			else if(str->at(s.pos) == '\n')
			{
				++s.line;
				s.charpos = 0;
			}
			else
				++s.charpos;
		}

		if(!ok)
			Throw("Missing closing alternate string '%s'.", ALTER_END);
	}
	else if(c == '-')
	{
		if(pos2 + 1 == str->length())
		{
			s.token = T_SYMBOL;
			s._char = c;
			s.item = c;
			s.pos = pos2 + 1;
		}
		else
		{
			++s.charpos;
			s.pos = pos2 + 1;
			int oldPos = s.pos;
			int oldCharpos = s.charpos;
			int oldLine = s.line;
			// znajdŸ nastêpny znak
			pos2 = FindFirstNotOf(s, returnEol ? " \t" : " \t\n\r", s.pos);
			if(pos2 == string::npos)
			{
				// same spacje, entery, taby
				// to koniec pliku
				s.token = T_SYMBOL;
				s._char = c;
				s.item = c;
			}
			else
			{
				char c = str->at(pos2);
				if(!(c >= '0' && c <= '9' && ParseNumber(s, pos2, true)))
				{
					// nie liczba, zwróc minus
					s.token = T_SYMBOL;
					s._char = '-';
					s.item = '-';
					s.pos = oldPos;
					s.charpos = oldCharpos;
					s.line = oldLine;
				}
			}
		}
	}
	else if(strchr(SYMBOLS, c))
	{
		if(c == '\'' && IsSet(flags, F_CHAR))
		{
			// char
			uint cp = s.charpos;
			s.pos = FindFirstOf(s, "'", pos2 + 1);

			if(s.pos == string::npos)
				formatter.Throw(Format("Not closed char ' opened at %u.", cp + 1));

			Unescape(*str, pos2 + 1, s.pos - pos2 - 1, s.item);

			if(s.item.empty())
				formatter.Throw(Format("Empty char sequence at %u.", cp + 1));
			if(s.item.size() > 1u)
				formatter.Throw(Format("Broken char sequence '%s' at %u.", s.item.c_str(), cp + 1));

			s.token = T_CHAR;
			s._char = s.item[0];
			++s.pos;
		}
		else
		{
			// symbol
			++s.charpos;
			s.pos = pos2 + 1;
			s.token = T_SYMBOL;
			s._char = c;
			s.item = c;
		}
	}
	else if(c >= '0' && c <= '9')
	{
		// number
		if(c == '0' && pos2 + 1 != str->size() && (str->at(pos2 + 1) == 'x' || str->at(pos2 + 1) == 'X'))
		{
			// hex number
			s.pos = FindFirstOf(s, WHITESPACE_SYMBOLS_DOT, pos2);
			if(pos2 == string::npos)
			{
				s.pos = str->length();
				s.item = str->substr(pos2);
			}
			else
				s.item = str->substr(pos2, s.pos - pos2);

			uint num = 0;
			for(uint i = 2; i < s.item.length(); ++i)
			{
				c = tolower(s.item[i]);
				if(c >= '0' && c <= '9')
				{
					num <<= 4;
					num += c - '0';
				}
				else if(c >= 'a' && c <= 'f')
				{
					num <<= 4;
					num += c - 'a' + 10;
				}
				else if(c >= 'A' && c <= 'F')
				{
					num <<= 4;
					num += c - 'A' + 10;
				}
				else
				{
					// broken hex number - item
					s.token = T_ITEM;
					return true;
				}
			}
			s.token = (num & 0x80000000) != 0 ? T_UINT : T_INT;
			s._int = num;
			s._float = (float)num;
			s._uint = num;
		}
		else
			ParseNumber(s, pos2, false);
	}
	else
	{
		// find end of this item
		bool ignoreDot = false;
		if(IsSet(flags, F_JOIN_DOT))
			ignoreDot = true;
		s.pos = FindFirstOf(s, ignoreDot ? WHITESPACE_SYMBOLS : WHITESPACE_SYMBOLS_DOT, pos2);
		if(pos2 == string::npos)
		{
			s.pos = str->length();
			s.item = str->substr(pos2);
		}
		else
			s.item = str->substr(pos2, s.pos - pos2);

		CheckItemOrKeyword(s, s.item);
	}

	return true;
}

//=================================================================================================
bool Tokenizer::ParseNumber(SeekData& s, uint pos2, bool negative)
{
	s.item.clear();
	if(negative)
		s.item = "-";
	int haveDot = 0;
	/*
	0 - number
	1 - number.
	2 - number.number
	3 - number.number.
	*/

	uint len = str->length();
	do
	{
		if(pos2 >= len)
		{
			// eof
			s.pos = pos2;
			break;
		}

		char c = str->at(pos2);
		if(c >= '0' && c <= '9')
		{
			s.item += c;
			if(haveDot == 1)
				haveDot = 2;
		}
		else if(c == '.')
		{
			if(haveDot == 0)
			{
				haveDot = 1;
				s.item += c;
			}
			else
			{
				// second dot, end parsing
				s.pos = pos2;
				break;
			}
		}
		else if(CharInStr(c, WHITESPACE_SYMBOLS) != 0)
		{
			// found symbol or whitespace, break
			s.pos = pos2;
			break;
		}
		else
		{
			if(haveDot == 0 || haveDot == 2)
			{
				// int item -> broken number
				// int . int item -> broken number
				// find end of item
				s.pos = FindFirstOf(s, WHITESPACE_SYMBOLS_DOT, pos2);
				if(s.pos == string::npos)
					s.item += str->substr(pos2);
				else
					s.item += str->substr(pos2, s.pos - pos2);
				s.token = T_ITEM;
				return false;
			}
			else if(haveDot == 1 || haveDot == 3)
			{
				// int dot item
				s.pos = pos2 - 1;
				s.item.pop_back();
				return true;
			}
		}

		++pos2;
		++s.charpos;
	}
	while(1);

	// parse number
	int64 val;
	TextHelper::ParseResult result = TextHelper::ToNumber(s.item.c_str(), val, s._float);
	if(result == TextHelper::Broken)
	{
		s.token = T_ITEM;
		return false;
	}
	if(result == TextHelper::Float)
	{
		s.token = T_FLOAT;
		s._int = (int)val;
		s._uint = (s._int < 0 ? 0 : s._int);
	}
	else if(val > std::numeric_limits<int>::max())
	{
		s.token = T_UINT;
		s._int = (int)val;
		s._uint = (uint)val;
	}
	else
	{
		s.token = T_INT;
		s._int = (int)val;
		s._uint = (s._int < 0 ? 0 : s._int);
	}
	return true;
}

//=================================================================================================
void Tokenizer::SetFlags(int _flags)
{
	flags = _flags;
	if(IsSet(flags, F_SEEK))
	{
		if(!seek)
			seek = new SeekData;
	}
	else
	{
		delete seek;
		seek = nullptr;
	}
}

//=================================================================================================
void Tokenizer::CheckItemOrKeyword(SeekData& s, const string& _item)
{
	Keyword k = { _item.c_str(), 0, 0 };
	auto end = keywords.end();
	auto it = std::lower_bound(keywords.begin(), end, k);
	if(it != end && _item == it->name)
	{
		// keyword
		s.token = T_KEYWORD;
		s.keyword.clear();
		if(it->enabled)
			s.keyword.push_back(&*it);
		if(IsSet(flags, F_MULTI_KEYWORDS))
		{
			do
			{
				++it;
				if(it == end || _item != it->name)
					break;
				if(it->enabled)
					s.keyword.push_back(&*it);
			}
			while(true);
		}
		if(s.keyword.empty())
			s.token = T_ITEM;
	}
	else
	{
		// normal text, item
		s.token = T_ITEM;
	}
}

//=================================================================================================
bool Tokenizer::NextLine()
{
	if(IsEof())
		return false;

	if(normalSeek.pos >= str->length())
	{
		normalSeek.token = T_EOF;
		return false;
	}

	uint pos2 = FindFirstNotOf(normalSeek, " \t", normalSeek.pos);
	if(pos2 == string::npos)
	{
		normalSeek.pos = string::npos;
		normalSeek.token = T_EOF;
		return false;
	}

	uint pos3 = FindFirstOf(normalSeek, "\n\r", pos2 + 1);
	if(pos3 == string::npos)
		normalSeek.item = str->substr(pos2);
	else
		normalSeek.item = str->substr(pos2, pos3 - pos2);

	normalSeek.token = T_ITEM;
	normalSeek.pos = pos3;
	return !normalSeek.item.empty();
}

//=================================================================================================
bool Tokenizer::PeekSymbol(char symbol)
{
	assert(normalSeek.token == T_SYMBOL || normalSeek.token == T_COMPOUND_SYMBOL);
	if(str->size() == normalSeek.pos)
		return false;
	char c = str->at(normalSeek.pos);
	if(c == symbol)
	{
		normalSeek.item += c;
		normalSeek._char = c;
		++normalSeek.charpos;
		++normalSeek.pos;
		normalSeek.token = T_COMPOUND_SYMBOL;
		return true;
	}
	else
		return false;
}

//=================================================================================================
uint Tokenizer::FindFirstNotOf(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	uint len = strlen(_str);
	for(uint i = _start, end = str->length(); i < end; ++i)
	{
		char c = str->at(i);
		bool found = false;

		for(uint j = 0; j < len; ++j)
		{
			if(c == _str[j])
			{
				found = true;
				break;
			}
		}

		if(!found)
			return i;

		if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOf(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	uint len = strlen(_str);
	for(uint i = _start, end = str->length(); i < end; ++i)
	{
		char c = str->at(i);

		for(uint j = 0; j < len; ++j)
		{
			if(c == _str[j])
				return i;
		}

		if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindFirstOfStr(SeekData& s, cstring _str, uint _start)
{
	assert(_start < str->length());

	for(uint i = _start, end = str->length(); i < end; ++i)
	{
		char c = str->at(i);
		if(c == _str[0])
		{
			cstring _s = _str;
			while(true)
			{
				++s.charpos;
				++i;
				++_s;
				if(*_s == 0)
					return i;
				if(i == end)
					return string::npos;
				if(*_s != str->at(i))
					break;
			}
		}
		else if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
uint Tokenizer::FindEndOfQuote(SeekData& s, uint _start)
{
	if(_start >= str->length())
		return string::npos;

	for(uint i = _start, end = str->length(); i < end; ++i)
	{
		char c = str->at(i);

		if(c == '"')
		{
			if(i == _start || str->at(i - 1) != '\\')
				return i;
		}
		else if(c == '\n')
		{
			++s.line;
			s.charpos = 0;
		}
		else
			++s.charpos;
	}

	return string::npos;
}

//=================================================================================================
const Keyword* Tokenizer::FindKeyword(int _id, int _group) const
{
	for(vector<Keyword>::const_iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == _id && (it->group == _group || _group == EMPTY_GROUP))
			return &*it;
	}

	return nullptr;
}

//=================================================================================================
const KeywordGroup* Tokenizer::FindKeywordGroup(int group) const
{
	for(const KeywordGroup& g : groups)
	{
		if(g.id == group)
			return &g;
	}

	return nullptr;
}

//=================================================================================================
void Tokenizer::AddKeywords(int group, std::initializer_list<KeywordToRegister> const& toRegister, cstring groupName)
{
	for(const KeywordToRegister& k : toRegister)
		AddKeyword(k.name, k.id, group);

	if(toRegister.size() > 0)
		needSorting = true;

	if(groupName)
		AddKeywordGroup(groupName, group);
}

//=================================================================================================
bool Tokenizer::RemoveKeyword(cstring name, int id, int group)
{
	Keyword k = { name, 0, 0 };
	auto end = keywords.end();
	auto it = std::lower_bound(keywords.begin(), end, k);
	if(it != end && strcmp(name, it->name) == 0)
	{
		if(it->id == id && it->group == group)
		{
			// found
			keywords.erase(it);
			return true;
		}

		// not found exact id/group, if multikeywords check next items
		if(IsSet(flags, F_MULTI_KEYWORDS))
		{
			do
			{
				++it;
				if(it == end || strcmp(it->name, name) != 0)
					break;
				if(it->id == id && it->group == group)
				{
					keywords.erase(it);
					return true;
				}
			}
			while(true);
		}
	}

	return false;
}

//=================================================================================================
bool Tokenizer::RemoveKeyword(int id, int group)
{
	for(vector<Keyword>::iterator it = keywords.begin(), end = keywords.end(); it != end; ++it)
	{
		if(it->id == id && it->group == group)
		{
			keywords.erase(it);
			return true;
		}
	}

	return false;
}

//=================================================================================================
bool Tokenizer::RemoveKeywordGroup(int group)
{
	auto end = groups.end();
	KeywordGroup toFind = { nullptr, group };
	auto it = std::find(groups.begin(), end, toFind);
	if(it == end)
		return false;
	groups.erase(it);
	return true;
}

//=================================================================================================
void Tokenizer::EnableKeywordGroup(int group)
{
	for(Keyword& k : keywords)
	{
		if(k.group == group)
			k.enabled = true;
	}
}

//=================================================================================================
void Tokenizer::DisableKeywordGroup(int group)
{
	for(Keyword& k : keywords)
	{
		if(k.group == group)
			k.enabled = false;
	}
}

//=================================================================================================
cstring Tokenizer::FormatToken(TOKEN token, int* what, int* what2)
{
	cstring name = GetTokenName(token);
	if(what == nullptr)
		return name;

	switch(token)
	{
	case T_ITEM:
	case T_STRING:
	case T_TEXT:
		return Format("%s '%s'", name, (cstring)what);
	case T_CHAR:
		return Format("%s '%s'", name, EscapeChar(*(char*)what));
	case T_SYMBOL:
		return Format("%s '%c'", name, *(char*)what);
	case T_INT:
	case T_BOOL:
		return Format("%s %d", name, *what);
	case T_UINT:
		return Format("%s %u", name, *(uint*)what);
	case T_FLOAT:
		return Format("%s %g", name, *(float*)what);
	case T_KEYWORD:
		{
			const Keyword* keyword = FindKeyword(*what, what2 ? *what2 : EMPTY_GROUP);
			const KeywordGroup* group = (what2 ? FindKeywordGroup(*what2) : nullptr);
			if(keyword)
			{
				if(group)
				{
					if(IsSet(flags, F_HIDE_ID))
						return Format("%s '%s' from group '%s'", name, keyword->name, group->name);
					else
						return Format("%s '%s'(%d) from group '%s'(%d)", name, keyword->name, keyword->id, group->name, group->id);
				}
				else if(what2)
				{
					if(IsSet(flags, F_HIDE_ID))
						return Format("%s '%s' from group %d", name, keyword->name, *what2);
					else
						return Format("%s '%s'(%d) from group %d", name, keyword->name, keyword->id, *what2);
				}
				else
				{
					if(IsSet(flags, F_HIDE_ID))
						return Format("%s '%s'", name, keyword->name);
					else
						return Format("%s '%s'(%d)", name, keyword->name, keyword->id);
				}
			}
			else
			{
				if(group)
				{
					if(IsSet(flags, F_HIDE_ID))
						return Format("missing %s %d from group '%s'", name, *what, group->name);
					else
						return Format("missing %s %d from group '%s'(%d)", name, *what, group->name, group->id);
				}
				else if(what2)
					return Format("missing %s %d from group %d", name, *what, *what2);
				else
					return Format("missing %s %d", name, *what);
			}
		}
		break;
	case T_KEYWORD_GROUP:
		{
			const KeywordGroup* group = FindKeywordGroup(*what);
			if(group)
			{
				if(IsSet(flags, F_HIDE_ID))
					return Format("%s '%s'", name, group->name);
				else
					return Format("%s '%s'(%d)", name, group->name, group->id);
			}
			else
				return Format("%s %d", name, *what);
		}
	case T_SYMBOLS_LIST:
		return Format("%s \"%s\"", name, (cstring)what);
	default:
		return "missing";
	}
}

//=================================================================================================
void Tokenizer::CheckSorting()
{
	if(!needSorting)
		return;

	needSorting = false;
	std::sort(keywords.begin(), keywords.end());

	if(!IsSet(flags, F_MULTI_KEYWORDS))
	{
		assert(CheckMultiKeywords());
	}
}

//=================================================================================================
bool Tokenizer::CheckMultiKeywords() const
{
	int errors = 0;

	const Keyword* prev = &keywords[0];
	for(uint i = 1; i < keywords.size(); ++i)
	{
		if(strcmp(keywords[i].name, prev->name) == 0)
		{
			++errors;
			Error("Keyword '%s' multiple definitions (%d,%d) and (%d,%d).", prev->name, prev->id, prev->group,
				keywords[i].id, keywords[i].group);
		}
		prev = &keywords[i];
	}

	if(errors > 0)
	{
		Error("Multiple keywords %d with same id. Use F_MULTI_KEYWORDS or fix that.", errors);
		return false;
	}
	else
		return true;
}

//=================================================================================================
void Tokenizer::ParseFlags(int group, int& flags)
{
	if(IsSymbol('|'))
		Next();
	else
		flags = 0;

	if(IsSymbol('{'))
	{
		Next();

		do
		{
			if(IsSymbol('~'))
			{
				Next();
				ClearBit(flags, MustGetKeywordId(group));
			}
			else
				flags |= MustGetKeywordId(group);
			Next();
		}
		while(!IsSymbol('}'));
	}
	else
	{
		if(IsSymbol('~'))
		{
			Next();
			ClearBit(flags, MustGetKeywordId(group));
		}
		else
			flags |= MustGetKeywordId(group);
	}
}

//=================================================================================================
void Tokenizer::ParseFlags(std::initializer_list<FlagGroup> const& flags)
{
	if(IsSymbol('|'))
		Next();
	else
	{
		for(FlagGroup const& f : flags)
			*f.flags = 0;
	}

	bool unexpected = false;

	if(IsSymbol('{'))
	{
		Next();

		do
		{
			bool found = false;
			bool neg = IsSymbol('~');
			if(neg)
				Next();

			for(FlagGroup const& f : flags)
			{
				if(IsKeywordGroup(f.group))
				{
					if(neg)
						ClearBit(*f.flags, GetKeywordId(f.group));
					else
						*f.flags |= GetKeywordId(f.group);
					found = true;
					break;
				}
			}

			if(!found)
			{
				unexpected = true;
				break;
			}

			Next();
		}
		while(!IsSymbol('}'));
	}
	else
	{
		bool found = false;
		bool neg = IsSymbol('~');
		if(neg)
			Next();

		for(FlagGroup const& f : flags)
		{
			if(IsKeywordGroup(f.group))
			{
				if(neg)
					ClearBit(*f.flags, GetKeywordId(f.group));
				else
					*f.flags |= GetKeywordId(f.group);
				found = true;
				break;
			}
		}

		if(!found)
			unexpected = true;
	}

	if(unexpected)
	{
		auto& formatter = StartUnexpected();

		for(FlagGroup const& f : flags)
		{
			int g = f.group;
			formatter.Add(T_KEYWORD_GROUP, &g);
		}

		formatter.Throw();
	}
}

//=================================================================================================
void Tokenizer::Parse(float* f, uint count)
{
	AssertSymbol('{');
	Next();
	for(uint i = 0; i < count; ++i)
	{
		f[i] = MustGetFloat();
		Next();
	}
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Int2& i)
{
	if(IsSymbol('{'))
	{
		Next();
		i.x = MustGetInt();
		Next();
		i.y = MustGetInt();
		Next();
		AssertSymbol('}');
		Next();
	}
	else
	{
		i.x = i.y = MustGetInt();
		Next();
	}
}

//=================================================================================================
void Tokenizer::Parse(Rect& rect)
{
	AssertSymbol('{');
	Next();
	rect.p1.x = MustGetInt();
	Next();
	rect.p1.y = MustGetInt();
	Next();
	rect.p2.x = MustGetInt();
	Next();
	rect.p2.y = MustGetInt();
	Next();
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Vec2& v)
{
	if(IsSymbol('{'))
	{
		Next();
		v.x = MustGetFloat();
		Next();
		v.y = MustGetFloat();
		Next();
		AssertSymbol('}');
		Next();
	}
	else
	{
		v.x = v.y = MustGetFloat();
		Next();
	}
}

//=================================================================================================
void Tokenizer::Parse(Vec3& v)
{
	AssertSymbol('{');
	Next();
	v.x = MustGetFloat();
	Next();
	v.y = MustGetFloat();
	Next();
	v.z = MustGetFloat();
	Next();
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Vec4& v)
{
	AssertSymbol('{');
	Next();
	v.x = MustGetFloat();
	Next();
	v.y = MustGetFloat();
	Next();
	v.z = MustGetFloat();
	Next();
	if(IsSymbol('}'))
	{
		v.w = 1.f;
		return;
	}
	v.w = MustGetFloat();
	Next();
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Box2d& box)
{
	AssertSymbol('{');
	Next();
	Parse(box.v1);
	Parse(box.v2);
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Box& box)
{
	AssertSymbol('{');
	Next();
	Parse(box.v1);
	Parse(box.v2);
	AssertSymbol('}');
	Next();
}

//=================================================================================================
void Tokenizer::Parse(Color& c)
{
	if(IsSymbol('{'))
	{
		Next();
		c.r = MustGetInt();
		Next();
		c.g = MustGetInt();
		Next();
		c.b = MustGetInt();
		Next();
		if(!IsSymbol('}'))
		{
			c.a = MustGetInt();
			Next();
		}
		else
			c.a = 255;
		AssertSymbol('}');
		Next();
	}
	else if(IsItem("BLACK"))
	{
		c = Color::Black;
		Next();
	}
	else
		Unexpected("color");
}

//=================================================================================================
const string& Tokenizer::GetBlock(char open, char close, bool includeSymbol)
{
	AssertSymbol(open);
	int opened = 1;
	uint blockStart = normalSeek.pos - 1;
	while(Next())
	{
		if(IsSymbol(open))
			++opened;
		else if(IsSymbol(close))
		{
			--opened;
			if(opened == 0)
			{
				if(includeSymbol)
					normalSeek.item = str->substr(blockStart, normalSeek.pos - blockStart);
				else
					normalSeek.item = str->substr(blockStart + 1, normalSeek.pos - blockStart - 2);
				return normalSeek.item;
			}
		}
	}

	int symbol = (int)open;
	Unexpected(T_SYMBOL, &symbol);
}

//=================================================================================================
bool Tokenizer::IsSymbol(cstring s, char* c) const
{
	assert(s);
	if(!IsSymbol())
		return false;
	char c2, symbol = GetSymbol();
	while((c2 = *s) != 0)
	{
		if(c2 == symbol)
		{
			if(c)
				*c = symbol;
			return true;
		}
		++s;
	}
	return false;
}

//=================================================================================================
char Tokenizer::MustGetSymbol(cstring symbols) const
{
	assert(symbols);
	char c;
	if(IsSymbol(symbols, &c))
		return c;
	Unexpected(T_SYMBOLS_LIST, (int*)symbols);
}

//=================================================================================================
bool Tokenizer::SeekStart(bool returnEol)
{
	assert(seek);
	seek->token = normalSeek.token;
	seek->pos = normalSeek.pos;
	seek->line = normalSeek.line;
	seek->charpos = normalSeek.charpos;
	return SeekNext(returnEol);
}

//=================================================================================================
cstring Tokenizer::GetTokenName(TOKEN _tt)
{
	switch(_tt)
	{
	case T_NONE:
		return "none";
	case T_EOF:
		return "end of file";
	case T_EOL:
		return "end of line";
	case T_ITEM:
		return "item";
	case T_STRING:
		return "string";
	case T_CHAR:
		return "char";
	case T_SYMBOL:
		return "symbol";
	case T_INT:
		return "integer";
	case T_UINT:
		return "unsigned";
	case T_FLOAT:
		return "float";
	case T_KEYWORD:
		return "keyword";
	case T_KEYWORD_GROUP:
		return "keyword group";
	case T_TEXT:
		return "text";
	case T_BOOL:
		return "bool";
	case T_SYMBOLS_LIST:
		return "symbols list";
	case T_COMPOUND_SYMBOL:
		return "compound symbol";
	default:
		assert(0);
		return "unknown";
	}
}

//=================================================================================================
cstring Tokenizer::GetTokenValue(const SeekData& s) const
{
	cstring name = GetTokenName(s.token);

	switch(s.token)
	{
	case T_ITEM:
	case T_STRING:
	case T_COMPOUND_SYMBOL:
		return Format("%s '%s'", name, s.item.c_str());
	case T_CHAR:
		return Format("%s '%c'", name, EscapeChar(s._char));
	case T_SYMBOL:
		return Format("%s '%c'", name, s._char);
	case T_INT:
		return Format("%s %d", name, s._int);
	case T_UINT:
		return Format("%s %u", name, s._uint);
	case T_FLOAT:
		return Format("%s %g", name, s._float);
	case T_KEYWORD:
		{
			LocalString str = name;
			str += " '";
			str += s.item;
			str += '\'';
			if(s.keyword.size() == 1u)
			{
				Keyword& keyword = *s.keyword[0];
				const KeywordGroup* group = FindKeywordGroup(keyword.group);
				if(!IsSet(flags, F_HIDE_ID))
					str += Format("(%d)", keyword.id);
				if(group)
				{
					str += Format(" from group '%s'", group->name);
					if(!IsSet(flags, F_HIDE_ID))
						str += Format("(%d)", group->id);
				}
				else if(keyword.group != EMPTY_GROUP)
					str += Format(" from group %d", keyword.group);
			}
			else
			{
				str += " in multiple groups {";
				bool first = true;
				for(Keyword* k : s.keyword)
				{
					if(first)
						first = false;
					else
						str += ", ";
					const KeywordGroup* group = FindKeywordGroup(k->group);
					if(group)
					{
						if(IsSet(flags, F_HIDE_ID))
							str += Format("'%s'", group->name);
						else
							str += Format("[%d,'%s'(%d)]", k->id, group->name, group->id);
					}
					else
						str += Format("[%d:%d]", k->id, k->group);
				}
				str += "}";
			}
			return Format("%s", str.c_str());
		}
	case T_NONE:
	case T_EOF:
	case T_EOL:
	default:
		return name;
	}
}

//=================================================================================================
int Tokenizer::IsKeywordGroup(std::initializer_list<int> const& groups) const
{
	if(!IsKeyword())
		return MISSING_GROUP;
	for(int group : groups)
	{
		for(Keyword* k : normalSeek.keyword)
		{
			if(k->group == group)
				return group;
		}
	}
	return MISSING_GROUP;
}

//=================================================================================================
Pos Tokenizer::GetPos()
{
	Pos p{};
	p.line = normalSeek.line + 1;
	p.charpos = normalSeek.charpos + 1;
	p.pos = normalSeek.startPos;
	return p;
}

//=================================================================================================
void Tokenizer::MoveTo(const Pos& p)
{
	normalSeek.pos = p.pos;
	normalSeek.token = T_NONE;
	DoNext(normalSeek, false);
	normalSeek.line = p.line - 1;
	normalSeek.charpos = p.charpos - 1;
}

//=================================================================================================
char Tokenizer::GetClosingSymbol(char start)
{
	switch(start)
	{
	case '(':
		return ')';
	case '{':
		return '}';
	case '[':
		return ']';
	case '<':
		return '>';
	default:
		return 0;
	}
}

//=================================================================================================
bool Tokenizer::MoveToClosingSymbol(char start, char end)
{
	if(end == 0)
	{
		end = GetClosingSymbol(start);
		assert(end);
	}

	int depth = 1;
	if(!IsSymbol(start))
		return false;

	while(Next())
	{
		if(IsSymbol(start))
			++depth;
		else if(IsSymbol(end))
		{
			if(--depth == 0)
				return true;
		}
	}

	return false;
}

//=================================================================================================
void Tokenizer::ForceMoveToClosingSymbol(char start, char end)
{
	if(end == 0)
	{
		end = GetClosingSymbol(start);
		assert(end);
	}

	uint pos = GetLine(),
		charpos = GetCharPos();
	if(!MoveToClosingSymbol(start, end))
		Throw("Missing closing '%c' started at '%c' (%u:%u).", end, start, pos, charpos);
}

//=================================================================================================
cstring Tokenizer::GetTextRest()
{
	if(normalSeek.pos == string::npos)
		return "";
	return str->c_str() + normalSeek.pos;
}

//=================================================================================================
SeekData& Tokenizer::Query()
{
	static SeekData tmp;
	tmp.token = normalSeek.token;
	tmp.pos = normalSeek.pos;
	tmp.line = normalSeek.line;
	tmp.charpos = normalSeek.charpos;
	DoNext(tmp, false);
	return tmp;
}

//=================================================================================================
int Tokenizer::ParseTop(int group, delegate<bool(int)> action)
{
	int errors = 0;

	try
	{
		Next();

		while(!IsEof())
		{
			bool skip = false;

			if(IsKeywordGroup(group))
			{
				int top = GetKeywordId(group);
				Next();
				if(!action(top))
					skip = true;
			}
			else
			{
				Error(FormatUnexpected(T_KEYWORD_GROUP, &group));
				skip = true;
			}

			if(skip)
			{
				SkipToKeywordGroup(group);
				++errors;
			}
			else
				Next();
		}
	}
	catch(const Exception& e)
	{
		Error("Failed to parse top group: %s", e.ToString());
		++errors;
	}

	return errors;
}

//=================================================================================================
int Tokenizer::ParseTopId(int group, delegate<void(int, const string&)> action)
{
	int errors = 0;

	try
	{
		Next();

		while(!IsEof())
		{
			bool skip = false;

			if(IsKeywordGroup(group))
			{
				int top = GetKeywordId(group);
				Next();

				if(IsString())
				{
					tmpId = GetString();
					Next();

					try
					{
						action(top, tmpId);
					}
					catch(const Exception& ex)
					{
						Error("Failed to parse %s '%s': %s", FindKeyword(top, group)->name, tmpId.c_str(), ex.ToString());
						skip = true;
					}
				}
				else
				{
					Error(FormatUnexpected(T_STRING));
					skip = true;
				}
			}
			else
			{
				Error(FormatUnexpected(T_KEYWORD_GROUP, &group));
				skip = true;
			}

			if(skip)
			{
				SkipToKeywordGroup(group);
				++errors;
			}
			else
				Next();
		}
	}
	catch(const Exception& ex)
	{
		Error("Failed to parse top group: %s", ex.ToString());
		++errors;
	}

	return errors;
}
