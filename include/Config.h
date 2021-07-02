// configuration reader/writer
#pragma once

#include "Tokenizer.h"

//-----------------------------------------------------------------------------
class Config
{
	struct Entry
	{
		string name, value, cmdValue;
		bool haveCmdValue;
	};

public:
	enum Result
	{
		NO_FILE,
		PARSE_ERROR,
		OK
	};

	enum GetResult
	{
		GET_OK,
		GET_MISSING,
		GET_INVALID
	};

	template<typename T>
	struct EnumDef
	{
		cstring name;
		T value;
	};

	void Add(cstring name, cstring value);
	void Add(cstring name, const string& value) { Add(name, value.c_str()); }
	void Add(cstring name, bool value) { Add(name, value ? "1" : "0"); }
	void Add(cstring name, int value) { Add(name, Format("%d", value)); }
	void Add(cstring name, uint value) { Add(name, Format("%u", value)); }
	void Add(cstring name, float value) { Add(name, Format("%g", value)); }
	void Add(cstring name, const Int2& value) { Add(name, Format("{%d %d}", value.x, value.y)); }
	void ParseCommandLine(cstring cmdLine);
	Result Load(cstring defaultFilename);
	bool Save();
	void Remove(cstring name);
	void Rename(cstring name, cstring newName);

	const string& GetFileName() const { return fileName; }
	const string& GetError() const { return error; }
	bool GetBool(cstring name, bool def = false);
	const string& GetString(cstring name);
	const string& GetString(cstring name, const string& def);
	int GetInt(cstring name, int def = 0);
	uint GetUint(cstring name, uint def = 0);
	float GetFloat(cstring name, float def = 0.f);
	Int2 GetInt2(cstring name, Int2 def = Int2(0, 0));

	GetResult TryGetInt(cstring name, int& value);
	template<typename T>
	GetResult TryGetEnum(cstring name, T& value, std::initializer_list<EnumDef<T>> const & enum_defs)
	{
		Entry* e = GetEntry(name);
		if(!e)
			return GET_MISSING;
		else
		{
			for(const EnumDef<T>& en : enum_defs)
			{
				if(e->value == en.name)
				{
					value = en.value;
					return GET_OK;
				}
			}
			return GET_MISSING;
		}
	}

private:
	Entry* GetEntry(cstring name);
	string* GetEntryValue(cstring name);

	vector<Entry> entries;
	string fileName, tmpstr, error;
	Tokenizer t;
	bool changes;
};
