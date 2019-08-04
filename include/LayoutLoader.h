#pragma once

//-----------------------------------------------------------------------------
#include "Tokenizer.h"

//-----------------------------------------------------------------------------
class LayoutLoader
{
	struct Entry
	{
		enum Type
		{
			AreaLayout,
			Font,
			Image,
			Color,
			Int2,
			Int
		} type;
		string name;
		uint offset;

		template<typename T>
		T& GetValue(byte* ptr)
		{
			return  *reinterpret_cast<T*>(ptr + offset);
		}
	};

	struct Control
	{
		Control(cstring name, uint size, std::type_index type) : name(name), size(size), type(type), used(false) {}
		void AddEntry(cstring name, Entry::Type type, uint offset)
		{
			Entry& entry = Add1(entries);
			entry.type = type;
			entry.name = name;
			entry.offset = offset;
		}
		Entry* FindEntry(const string& name)
		{
			for(Entry& e : entries)
			{
				if(e.name == name)
					return &e;
			}
			return nullptr;
		}

		string name;
		vector<Entry> entries;
		uint size;
		std::type_index type;
		bool used;
	};

public:
	LayoutLoader(Gui* gui) : gui(gui), initialized(false) {}
	~LayoutLoader();
	Layout* LoadFromFile(cstring path);
	Font* GetFont(cstring name)
	{
		Font* font = FindFont(name);
		assert(font);
		return font;
	}

private:
	void RegisterKeywords();
	void RegisterControls();
	Control* AddControl(cstring name, uint size, const type_info& type);
	template<typename T>
	Control* AddControl(cstring name)
	{
		return AddControl(name, sizeof(T), typeid(T));
	}
	Control* FindControl(const string& name);
	Font* FindFont(const string& name);
	void ParseFont(const string& name);
	void ParseControl(const string& name);

	Gui* gui;
	Layout* layout;
	std::map<string, Font*> fonts;
	std::map<string, Control*> controls;
	Tokenizer t;
	bool initialized;
};
