// configuration reader/writer
#include "Pch.h"
#include "Config.h"
#include "File.h"

//=================================================================================================
void Config::Add(cstring name, cstring value)
{
	assert(name && value);

	for(Entry& entry : entries)
	{
		if(entry.name == name)
		{
			if(entry.value != value)
			{
				entry.value = value;
				changes = true;
			}
			return;
		}
	}

	Entry& e = Add1(entries);
	e.name = name;
	e.value = value;
	e.haveCmdValue = false;

	changes = true;
}

//=================================================================================================
Config::Entry* Config::GetEntry(cstring name)
{
	assert(name);
	for(Entry& entry : entries)
	{
		if(entry.name == name)
			return &entry;
	}
	return nullptr;
}

//=================================================================================================
string* Config::GetEntryValue(cstring name)
{
	Entry* entry = GetEntry(name);
	if(entry)
		return entry->haveCmdValue ? &entry->cmdValue : &entry->value;
	return nullptr;
}

//=================================================================================================
bool Config::GetBool(cstring name, bool def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;

	if(OR3_EQ(*value, "0", "false", "FALSE"))
		return false;
	else if(OR3_EQ(*value, "1", "true", "TRUE"))
		return true;
	else
		return def;
}

//=================================================================================================
const string& Config::GetString(cstring name)
{
	string* value = GetEntryValue(name);
	if(!value)
	{
		tmpstr.clear();
		return tmpstr;
	}
	else
		return *value;
}

//=================================================================================================
const string& Config::GetString(cstring name, const string& def)
{
	string* value = GetEntryValue(name);
	if(!value)
	{
		tmpstr = def;
		return tmpstr;
	}
	else
		return *value;
}

//=================================================================================================
int Config::GetInt(cstring name, int def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;

	int result;
	if(TextHelper::ToInt(value->c_str(), result))
		return result;
	else
		return def;
}

//=================================================================================================
int Config::GetEnumValue(cstring name, std::initializer_list<std::pair<cstring, int>> const& values, int def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;

	for(const std::pair<cstring, int>& v : values)
	{
		if(*value == v.first)
			return v.second;
	}

	return def;
}

//=================================================================================================
uint Config::GetUint(cstring name, uint def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;

	uint result;
	if(TextHelper::ToUint(value->c_str(), result))
		return result;
	else
		return def;
}

//=================================================================================================
float Config::GetFloat(cstring name, float def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;
	else
		return (float)atof(value->c_str());
}

//=================================================================================================
Int2 Config::GetInt2(cstring name, Int2 def)
{
	string* value = GetEntryValue(name);
	if(!value)
		return def;

	// old syntax compatibility "800x600"
	if(value->at(0) != '{')
	{
		Int2 result;
		if(sscanf_s(value->c_str(), "%dx%d", &result.x, &result.y) != 2)
		{
			Warn("Invalid Int2 '%s' value '%s'.", name, value->c_str());
			return def;
		}
		return result;
	}

	// new syntax {800 600}
	t.FromString(*value);
	try
	{
		Int2 result;
		t.Next();
		t.Parse(result);
		t.AssertEof();
		return result;
	}
	catch(const Tokenizer::Exception&)
	{
		Warn("Invalid Int2 '%s' value '%s'.", name, value->c_str());
		return def;
	}
}

//=================================================================================================
void Config::ParseCommandLine(cstring cmdLine)
{
	assert(cmdLine);

	const vector<string> parts = Split(cmdLine);
	LocalString name, value;

	for(auto it = parts.begin(), end = parts.end(); it != end;)
	{
		if(it->at(0) != '-')
		{
			Error("Config: Invalid command line parameter '%s'.", it->c_str());
			break;
		}
		name = it->substr(1);

		++it;
		value = "1";
		if(it != end && it->at(0) != '-')
		{
			value = *it;
			++it;
		}

		bool exists = false;
		for(Entry& entry : entries)
		{
			if(name == entry.name)
			{
				entry.cmdValue = (string&)value;
				exists = false;
			}
		}

		if(!exists)
		{
			Entry& entry = Add1(entries);
			entry.name = (string&)name;
			entry.cmdValue = (string&)value;
			entry.haveCmdValue = true;
		}
	}

	fileName = GetString("config");
}

//=================================================================================================
Config::Result Config::Load(cstring defaultFilename)
{
	assert(defaultFilename);

	if(fileName.empty())
		fileName = defaultFilename;

	if(!t.FromFile(fileName))
	{
		changes = true;
		return NO_FILE;
	}

	LocalString name, value;

	try
	{
		t.Next();

		// version info
		if(t.IsSymbol('#'))
		{
			t.Next();
			t.AssertItem("version");
			t.Next();
			t.AssertInt();
			t.Next();
		}

		// configuration
		while(!t.IsEof())
		{
			// name
			name = t.MustGetItem();
			t.Next();

			// =
			t.AssertSymbol('=');
			t.Next(true);

			// value
			if(t.IsEol())
				value->clear();
			else
			{
				if(t.IsSymbol('{'))
					value = t.ParseBlock();
				else
					value = t.GetTokenString();
			}

			// add if not exists
			bool exists = false;
			for(Entry& entry : entries)
			{
				if(name == entry.name)
				{
					entry.value = (string&)value;
					exists = false;
				}
			}

			if(!exists)
			{
				Entry& entry = Add1(entries);
				entry.name = (string&)name;
				entry.value = (string&)value;
				entry.haveCmdValue = false;
			}

			t.Next();
		}
	}
	catch(const Tokenizer::Exception& e)
	{
		error = e.ToString();
		return PARSE_ERROR;
	}

	changes = false;
	return OK;
}

//=================================================================================================
bool Config::Save()
{
	if(fileName.empty())
		return false;

	if(!changes)
		return true;

	TextWriter f(fileName);
	if(!f)
		return false;

	std::sort(entries.begin(), entries.end(), [](const Config::Entry& e1, const Config::Entry& e2) -> bool { return e1.name < e2.name; });

	for(Entry& entry : entries)
	{
		if(entry.value.empty())
			continue;

		cstring s;
		if(entry.value[0] != '{' && entry.value.find_first_of(" \n\t\\,./;'[]-=<>?:\"{}!@#$%^&*()_+") != string::npos)
			s = Format("%s = \"%s\"\n", entry.name.c_str(), Escape(entry.value));
		else
			s = Format("%s = %s\n", entry.name.c_str(), entry.value.c_str());
		f << s;
	}

	changes = false;
	return true;
}

//=================================================================================================
void Config::Remove(cstring name)
{
	assert(name);
	for(vector<Entry>::iterator it = entries.begin(), end = entries.end(); it != end; ++it)
	{
		if(it->name == name)
		{
			entries.erase(it);
			changes = true;
			return;
		}
	}
}

//=================================================================================================
void Config::Rename(cstring name, cstring newName)
{
	assert(name && newName);
	Entry* e = GetEntry(name);
	if(e)
	{
		e->name = newName;
		changes = true;
	}
}
