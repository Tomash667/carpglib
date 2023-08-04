#pragma once

namespace tokenizer
{
	enum TOKEN
	{
		T_NONE,
		T_EOF,
		T_EOL,
		T_ITEM,
		T_STRING,
		T_CHAR,
		T_SYMBOL,
		T_INT,
		T_UINT,
		T_FLOAT,
		T_KEYWORD,
		T_KEYWORD_GROUP,
		T_TEXT,
		T_BOOL,
		T_SYMBOLS_LIST,
		T_COMPOUND_SYMBOL
	};

	static const int EMPTY_GROUP = -1;
	static const int MISSING_GROUP = -2;

	struct Keyword
	{
		cstring name;
		int id, group;
		bool enabled;

		bool operator < (const Keyword& k)
		{
			return strcmp(name, k.name) < 0;
		}
	};

	struct KeywordGroup
	{
		cstring name;
		int id;

		bool operator == (const KeywordGroup& group) const
		{
			return id == group.id;
		}
	};

	struct KeywordToRegister
	{
		cstring name;
		int id;
	};

	template<typename T>
	struct KeywordToRegisterEnum
	{
		cstring name;
		T id;
	};

	struct SeekData
	{
		uint pos, startPos, line, charpos;
		string item;
		TOKEN token;
		int _int;
		float _float;
		char _char;
		uint _uint;
		vector<Keyword*> keyword;
	};

	struct Pos
	{
		uint pos, line, charpos;
	};

	struct FlagGroup
	{
		int* flags;
		int group;
	};

	//-----------------------------------------------------------------------------
	// Tokenizer
	//-----------------------------------------------------------------------------
	class Tokenizer
	{
		friend class Formatter;

	public:
		static const int EMPTY_GROUP = -1;

		class Exception
		{
		public:
			uint line, charpos;
			string* str, *filename;

			cstring ToString() const
			{
				return Format("%s(%d:%d) %s", filename ? filename->c_str() : "", line, charpos, str->c_str());
			}
		};

		class Formatter
		{
			friend class Tokenizer;

		public:
			Formatter& Add(TOKEN token, int* what = nullptr, int* what2 = nullptr)
			{
				if(count > 0)
					s += ", ";
				s += t->FormatToken(token, what, what2);
				++count;
				return *this;
			}

			Formatter& AddList(TOKEN token, std::initializer_list<int> const& items)
			{
				if(count > 0)
					s += ", ";
				s += t->FormatToken(token);
				s += "{";
				for(int item : items)
					s += Format("%d, ", item);
				s.pop_back(); s.pop_back(); // delete the last two characters (", ")
				s += "}";
				++count;
				return *this;
			}

			__declspec(noreturn) void Throw()
			{
				End();
				OnThrow();
				throw e;
			}

			cstring Get()
			{
				End();
				return Format("(%d:%d) %s", e.line, e.charpos, s.c_str());
			}

			const string& GetNoLineNumber()
			{
				End();
				return s;
			}

		private:
			explicit Formatter(Tokenizer* t) : t(t)
			{
				e.str = &s;
				sd = &t->normalSeek;
			}

			void SetFilename()
			{
				if(IsSet(t->flags, Tokenizer::F_FILE_INFO))
					e.filename = &t->filename;
				else
					e.filename = nullptr;
			}

			void Prepare()
			{
				e.line = t->GetLine();
				e.charpos = t->GetCharPos();
				SetFilename();
			}

			void Start()
			{
				Prepare();
				e.str = &s;
				s = "Expecting ";
				count = 0;
			}

			void End()
			{
				s += Format(", found %s.", t->GetTokenValue(*sd));
			}

			__declspec(noreturn) void Throw(cstring msg)
			{
				assert(msg);
				s = msg;
				Prepare();
				OnThrow();
				throw e;
			}

			__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg)
			{
				assert(msg);
				s = msg;
				e.line = line;
				e.charpos = charpos;
				SetFilename();
				OnThrow();
				throw e;
			}

			void OnThrow() {}

			Exception e;
			Tokenizer* t;
			const SeekData* sd;
			string s;
			int count;
		};

		enum FLAGS
		{
			F_JOIN_DOT = 1 << 0, // join dot after text (log.txt is one item, otherwise log dot txt - 2 items and symbol)
			F_MULTI_KEYWORDS = 1 << 1, // allows multiple keywords
			F_SEEK = 1 << 2, // allows to use seek operations
			F_FILE_INFO = 1 << 3, // add filename in errors
			F_CHAR = 1 << 4, // handle 'c' as char type (otherwise it's symbol ', identifier c, symbol ')
			F_HIDE_ID = 1 << 5, // in exceptions don't write keyword/group id, only name
		};

		explicit Tokenizer(int flags = 0);
		~Tokenizer();

		void FromString(cstring str);
		void FromString(const string& str);
		bool FromFile(cstring path);
		bool FromFile(const string& path) { return FromFile(path.c_str()); }
		void FromTokenizer(const Tokenizer& t);

		typedef bool(*SkipToFunc)(Tokenizer& t);

		bool Next(bool returnEol = false) { return DoNext(normalSeek, returnEol); }
		bool NextLine();
		bool SkipTo(delegate<bool(Tokenizer&)> f)
		{
			while(true)
			{
				if(f(*this))
					return true;

				if(!Next())
					return false;
			}
		}
		bool SkipToSymbol(char symbol)
		{
			return SkipTo([symbol](Tokenizer& t) { return t.IsSymbol(symbol); });
		}
		bool SkipToSymbol(cstring symbols)
		{
			return SkipTo([symbols](Tokenizer& t) { return t.IsSymbol(symbols); });
		}
		bool SkipToKeywordGroup(int group)
		{
			return SkipTo([group](Tokenizer& t) { return t.IsKeywordGroup(group); });
		}
		bool SkipToKeyword(int keyword, int group = EMPTY_GROUP)
		{
			return SkipTo([keyword, group](Tokenizer& t) { return t.IsKeyword(keyword, group); });
		}
		bool PeekSymbol(char symbol);
		char PeekChar()
		{
			if(IsEof())
				return 0;
			else
				return str->at(normalSeek.pos);
		}
		void NextChar()
		{
			++normalSeek.pos;
		}
		cstring FormatToken(TOKEN token, int* what = nullptr, int* what2 = nullptr);

		const Keyword* FindKeyword(int id, int group = EMPTY_GROUP) const;
		const KeywordGroup* FindKeywordGroup(int group) const;
		void AddKeyword(cstring name, int id, int group = EMPTY_GROUP)
		{
			assert(name);
			Keyword& k = Add1(keywords);
			k.name = name;
			k.id = id;
			k.group = group;
			k.enabled = true;
			needSorting = true;
		}
		void AddKeywordGroup(cstring name, int group)
		{
			assert(name);
			assert(group != EMPTY_GROUP);
			KeywordGroup newGroup = { name, group };
			assert(std::find(groups.begin(), groups.end(), newGroup) == groups.end());
			groups.push_back(newGroup);
		}
		// Add keyword for group in format {name, id}
		void AddKeywords(int group, std::initializer_list<KeywordToRegister> const& toRegister, cstring groupName = nullptr);
		template<typename T>
		void AddKeywords(int group, std::initializer_list<KeywordToRegisterEnum<T>> const& toRegister, cstring groupName = nullptr)
		{
			AddKeywords(group, (std::initializer_list<KeywordToRegister> const&)toRegister, groupName);
		}
		// Remove keyword (function with name is faster)
		bool RemoveKeyword(cstring name, int id, int group = EMPTY_GROUP);
		bool RemoveKeyword(int id, int group = EMPTY_GROUP);
		bool RemoveKeywordGroup(int group);
		void EnableKeywordGroup(int group);
		void DisableKeywordGroup(int group);

		Formatter& StartUnexpected() const { formatter.Start(); return formatter; }
		Formatter& SeekStartUnexpected() const
		{
			formatter.sd = seek;
			formatter.Start();
			return formatter;
		}
		__declspec(noreturn) void Unexpected(const SeekData& seekData) const
		{
			formatter.sd = &seekData;
			formatter.Throw(Format("Unexpected %s.", GetTokenValue(seekData)));
		}
		__declspec(noreturn) void Unexpected() const
		{
			Unexpected(normalSeek);
		}
		__declspec(noreturn) void SeekUnexpected() const
		{
			assert(seek);
			Unexpected(*seek);
		}
		__declspec(noreturn) void Unexpected(cstring err) const
		{
			assert(err);
			formatter.Throw(Format("Unexpected %s: %s", GetTokenValue(normalSeek), err));
		}
		__declspec(noreturn) void Unexpected(TOKEN expectedToken, int* what = nullptr, int* what2 = nullptr) const
		{
			StartUnexpected().Add(expectedToken, what, what2).Throw();
		}
		cstring FormatUnexpected(TOKEN expectedToken, int* what = nullptr, int* what2 = nullptr) const
		{
			return StartUnexpected().Add(expectedToken, what, what2).Get();
		}
		__declspec(noreturn) void Throw(cstring msg) const
		{
			formatter.Throw(msg);
		}
		template<typename T>
		__declspec(noreturn) void Throw(cstring msg, T arg, ...) const
		{
			cstring err = FormatList(msg, (va_list)&arg);
			formatter.Throw(err);
		}
		__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg) const
		{
			formatter.ThrowAt(line, charpos, msg);
		}
		template<typename T>
		__declspec(noreturn) void ThrowAt(uint line, uint charpos, cstring msg, T arg, ...) const
		{
			cstring err = FormatList(msg, (va_list)&arg);
			formatter.ThrowAt(line, charpos, err);
		}
		cstring Expecting(cstring what) const
		{
			return Format("Expecting %s, found %s.", what, GetTokenValue(normalSeek));
		}
		__declspec(noreturn) void ThrowExpecting(cstring what) const
		{
			formatter.Throw(Expecting(what));
		}

		//===========================================================================================================================
		bool IsToken(TOKEN _tt) const { return normalSeek.token == _tt; }
		bool IsEof() const { return IsToken(T_EOF); }
		bool IsEol() const { return IsToken(T_EOL); }
		bool IsItem() const { return IsToken(T_ITEM); }
		bool IsItem(cstring _item) const { return IsItem() && GetItem() == _item; }
		bool IsString() const { return IsToken(T_STRING); }
		bool IsChar() const { return IsToken(T_CHAR); }
		bool IsChar(char c) const { return IsChar() && GetChar() == c; }
		bool IsSymbol() const { return IsToken(T_SYMBOL); }
		bool IsSymbol(char c) const { return IsSymbol() && GetSymbol() == c; }
		bool IsSymbol(cstring s, char* c = nullptr) const;
		bool IsText() const { return IsItem() || IsString() || IsKeyword(); }
		bool IsInt() const { return IsToken(T_INT); }
		bool IsInt(int value) const { return IsInt() && GetInt() == value; }
		bool IsUint() const { return IsToken(T_UINT) || (IsInt() && GetInt() >= 0); }
		bool IsFloat() const { return IsToken(T_FLOAT) || IsToken(T_INT); }
		bool IsKeyword() const { return IsToken(T_KEYWORD); }
		bool IsKeyword(int id) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normalSeek.keyword)
				{
					if(k->id == id)
						return true;
				}
			}
			return false;
		}
		bool IsKeyword(int id, int group) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normalSeek.keyword)
				{
					if(k->id == id && k->group == group)
						return true;
				}
			}
			return false;
		}
		bool IsKeywordGroup(int group) const
		{
			if(IsKeyword())
			{
				for(Keyword* k : normalSeek.keyword)
				{
					if(k->group == group)
						return true;
				}
			}
			return false;
		}
		int IsAnyKeyword(int group, std::initializer_list<int> const& keywordIds)
		{
			for(auto keywordId : keywordIds)
			{
				if(IsKeyword(keywordId, group))
					return keywordId;
			}
			return -1;
		}
		int IsKeywordGroup(std::initializer_list<int> const& groups) const;
		bool IsBool() const { return IsInt() && (normalSeek._int == 0 || normalSeek._int == 1); }
		bool IsItemOrString() const { return IsItem() || IsString(); }

		//===========================================================================================================================
		static cstring GetTokenName(TOKEN _tt);
		// get formatted text of current token
		cstring GetTokenValue(const SeekData& s) const;
		cstring GetTokenValue() const
		{
			return GetTokenValue(normalSeek);
		}

		//===========================================================================================================================
		void AssertToken(TOKEN _tt) const
		{
			if(normalSeek.token != _tt)
				Unexpected(_tt);
		}
		void AssertEof() const { AssertToken(T_EOF); }
		void AssertItem() const { AssertToken(T_ITEM); }
		void AssertItem(cstring item) const
		{
			if(!IsItem(item))
				Unexpected(T_ITEM, (int*)item);
		}
		void AssertItemOrString() const
		{
			if(!IsItemOrString())
				Throw("Expected item or string.");
		}
		void AssertString() const { AssertToken(T_STRING); }
		void AssertChar() const { AssertToken(T_CHAR); }
		void AssertChar(char c) const
		{
			if(!IsChar(c))
				Unexpected(T_CHAR, (int*)&c);
		}
		void AssertSymbol() const { AssertToken(T_SYMBOL); }
		void AssertSymbol(char c) const
		{
			if(!IsSymbol(c))
				Unexpected(T_SYMBOL, (int*)&c);
		}
		void AssertInt() const { AssertToken(T_INT); }
		void AssertUint() const
		{
			if(!IsUint())
				AssertToken(T_UINT);
		}
		void AssertFloat() const
		{
			if(!IsFloat())
				AssertToken(T_FLOAT);
		}
		void AssertKeyword() const { AssertToken(T_KEYWORD); }
		void AssertKeyword(int id) const
		{
			if(!IsKeyword(id))
				Unexpected(T_KEYWORD, &id);
		}
		void AssertKeyword(int id, int group) const
		{
			if(!IsKeyword(id, group))
				Unexpected(T_KEYWORD, &id, &group);
		}
		void AssertKeywordGroup(int group) const
		{
			if(!IsKeywordGroup(group))
				Unexpected(T_KEYWORD_GROUP, &group);
		}
		int AssertKeywordGroup(std::initializer_list<int> const& groups) const
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups).Throw();
			return group;
		}
		void AssertText() const
		{
			if(!IsText())
				Unexpected(T_TEXT);
		}
		void AssertBool() const
		{
			if(!IsBool())
				Unexpected(T_BOOL);
		}

		//===========================================================================================================================
		TOKEN GetToken() const
		{
			return normalSeek.token;
		}
		cstring GetTokenName() const
		{
			return GetTokenName(GetToken());
		}
		const string& GetTokenString() const
		{
			return normalSeek.item;
		}
		const string& GetItem() const
		{
			assert(IsItem());
			return normalSeek.item;
		}
		const string& GetString() const
		{
			assert(IsString());
			return normalSeek.item;
		}
		char GetChar() const
		{
			assert(IsChar());
			return normalSeek._char;
		}
		char GetSymbol() const
		{
			assert(IsSymbol());
			return normalSeek._char;
		}
		int GetInt() const
		{
			assert(IsFloat());
			return normalSeek._int;
		}
		uint GetUint() const
		{
			assert(IsUint());
			return normalSeek._uint;
		}
		float GetFloat() const
		{
			assert(IsFloat());
			return normalSeek._float;
		}
		uint GetLine() const { return normalSeek.line + 1; }
		uint GetCharPos() const { return normalSeek.charpos + 1; }
		const string& GetInnerString() const { return *str; }
		const Keyword* GetKeyword() const
		{
			assert(IsKeyword());
			return normalSeek.keyword[0];
		}
		const Keyword* GetKeyword(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normalSeek.keyword)
			{
				if(k->id == id)
					return k;
			}
			return nullptr;
		}
		const Keyword* GetKeyword(int id, int group) const
		{
			assert(IsKeyword(id, group));
			for(Keyword* k : normalSeek.keyword)
			{
				if(k->id == id && k->group == group)
					return k;
			}
			return nullptr;
		}
		const Keyword* GetKeywordByGroup(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normalSeek.keyword)
			{
				if(k->group == group)
					return k;
			}
			return nullptr;
		}
		int GetKeywordId() const
		{
			assert(IsKeyword());
			return normalSeek.keyword[0]->id;
		}
		int GetKeywordId(int group) const
		{
			assert(IsKeywordGroup(group));
			for(Keyword* k : normalSeek.keyword)
			{
				if(k->group == group)
					return k->id;
			}
			return EMPTY_GROUP;
		}
		int GetKeywordGroup() const
		{
			assert(IsKeyword());
			return normalSeek.keyword[0]->group;
		}
		int GetKeywordGroup(int id) const
		{
			assert(IsKeyword(id));
			for(Keyword* k : normalSeek.keyword)
			{
				if(k->id == id)
					return k->group;
			}
			return EMPTY_GROUP;
		}
		const string& GetBlock(char open = '{', char close = '}', bool includeSymbol = true);
		const string& GetItemOrString() const
		{
			assert(IsItemOrString());
			return normalSeek.item;
		}
		const string& GetText() const
		{
			assert(IsText());
			return normalSeek.item;
		}

		//===========================================================================================================================
		const string& MustGetItem() const
		{
			AssertItem();
			return GetItem();
		}
		const string& MustGetString() const
		{
			AssertString();
			return GetString();
		}
		char MustGetChar() const
		{
			AssertChar();
			return GetChar();
		}
		char MustGetSymbol() const
		{
			AssertSymbol();
			return GetSymbol();
		}
		char MustGetSymbol(cstring symbols) const;
		int MustGetInt() const
		{
			AssertInt();
			return GetInt();
		}
		uint MustGetUint() const
		{
			AssertUint();
			return GetUint();
		}
		float MustGetFloat() const
		{
			AssertFloat();
			return GetFloat();
		}
		const Keyword* MustGetKeyword() const
		{
			AssertKeyword();
			return GetKeyword();
		}
		const Keyword* MustGetKeyword(int id) const
		{
			AssertKeyword(id);
			return GetKeyword(id);
		}
		const Keyword* MustGetKeyword(int id, int group) const
		{
			AssertKeyword(id, group);
			return GetKeyword(id, group);
		}
		const Keyword* MustGetKeywordByGroup(int group) const
		{
			AssertKeywordGroup(group);
			return GetKeywordByGroup(group);
		}
		int MustGetKeywordId() const
		{
			AssertKeyword();
			return GetKeywordId();
		}
		int MustGetKeywordId(int group) const
		{
			AssertKeywordGroup(group);
			return GetKeywordId(group);
		}
		template<typename T>
		T MustGetKeywordId(int group) const
		{
			int k = MustGetKeywordId(group);
			return static_cast<T>(k);
		}
		int MustGetKeywordGroup() const
		{
			AssertKeyword();
			return GetKeywordGroup();
		}
		int MustGetKeywordGroup(int id) const
		{
			AssertKeyword(id);
			return GetKeywordGroup(id);
		}
		template<typename T>
		T MustGetKeywordGroup(std::initializer_list<T> const& groups) const
		{
			int group = IsKeywordGroup(groups);
			if(group == MISSING_GROUP)
				StartUnexpected().AddList(T_KEYWORD_GROUP, groups).Throw();
			return (T)group;
		}
		const string& MustGetText() const
		{
			AssertText();
			return GetTokenString();
		}
		bool MustGetBool() const
		{
			AssertBool();
			return (normalSeek._int == 1);
		}
		const string& MustGetItemKeyword() const
		{
			if(!IsItem() && !IsKeyword())
				StartUnexpected().Add(T_ITEM).Add(T_KEYWORD).Throw();
			return normalSeek.item;
		}
		const string& MustGetItemOrString() const
		{
			AssertItemOrString();
			return normalSeek.item;
		}

		//===========================================================================================================================
		bool SeekStart(bool returnEol = false);
		bool SeekNext(bool returnEol = false)
		{
			assert(seek);
			return DoNext(*seek, returnEol);
		}
		bool SeekToken(TOKEN _token) const
		{
			assert(seek);
			return _token == seek->token;
		}
		bool SeekItem() const
		{
			return SeekToken(T_ITEM);
		}
		bool SeekSymbol() const
		{
			return SeekToken(T_SYMBOL);
		}
		bool SeekSymbol(char c) const
		{
			return SeekSymbol() && seek->_char == c;
		}

		//===========================================================================================================================
		SeekData& Query();
		bool QuerySymbol(char c) { SeekData& sd = Query(); return sd.token == T_SYMBOL && sd._char == c; }

		//===========================================================================================================================
		void Parse(float* f, uint count);
		template<uint N>
		void Parse(array<float, N>& arr)
		{
			Parse(arr.data(), N);
		}
		void Parse(Int2& i);
		void Parse(Rect& rect);
		void Parse(Vec2& v);
		void Parse(Vec3& v);
		void Parse(Vec4& v);
		void Parse(Box2d& box);
		void Parse(Box& box);
		void Parse(Color& c);
		const string& ParseBlock(char open = '{', char close = '}', bool include_symbol = true);
		void ParseFlags(int group, int& flags);
		void ParseFlags(std::initializer_list<FlagGroup> const& flags);
		int ParseTop(int group, delegate<bool(int)> action);
		int ParseTopId(int group, delegate<void(int, const string&)> action);

		void SetFlags(int _flags);
		void CheckItemOrKeyword(const string& item)
		{
			CheckSorting();
			CheckItemOrKeyword(normalSeek, item);
		}
		Pos GetPos();
		void MoveTo(const Pos& pos);
		char GetClosingSymbol(char start);
		bool MoveToClosingSymbol(char start, char end = 0);
		void ForceMoveToClosingSymbol(char start, char end = 0);
		void Reset()
		{
			normalSeek.token = T_NONE;
			normalSeek.pos = 0;
			normalSeek.line = 0;
			normalSeek.charpos = 0;
		}
		cstring GetTextRest();

	private:
		bool DoNext(SeekData& s, bool returnEol);
		void CheckItemOrKeyword(SeekData& s, const string& item);
		bool ParseNumber(SeekData& s, uint pos2, bool negative);
		uint FindFirstNotOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOf(SeekData& s, cstring _str, uint _start);
		uint FindFirstOfStr(SeekData& s, cstring _str, uint _start);
		uint FindEndOfQuote(SeekData& s, uint _start);
		void CheckSorting();
		bool CheckMultiKeywords() const;

		const string* str;
		int flags;
		string filename, tmpId;
		vector<Keyword> keywords;
		vector<KeywordGroup> groups;
		SeekData normalSeek;
		SeekData* seek;
		bool needSorting, ownString;
		mutable Formatter formatter;
	};
}
