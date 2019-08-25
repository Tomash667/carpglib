#pragma once

//-----------------------------------------------------------------------------
namespace EntitySystem
{
	extern bool clear;
}

//-----------------------------------------------------------------------------
template<typename T>
struct EntityType
{
	int id;

	struct Impl
	{
		int id_counter;
		std::unordered_map<int, T*> items;
		vector<pair<T**, int>> requests;
	};
	static Impl impl;

	EntityType() : id(-1) {}
	~EntityType()
	{
		if(id != -1 && !EntitySystem::clear)
			impl.items.erase(id);
	}
	void Register()
	{
		if(id == -1)
			id = impl.id_counter++;
		impl.items[id] = static_cast<T*>(this);
	}
	static T* GetById(int id)
	{
		if(id == -1)
			return nullptr;
		return impl.items[id];
	}
	static void AddRequest(T** e, int id)
	{
		impl.requests.push_back(std::make_pair(e, id));
	}
	static void ApplyRequests()
	{
		for(pair<T**, int>& request : impl.requests)
			*request.first = impl.items[request.second];
		impl.requests.clear();
	}
	static void ResetEntities()
	{
		impl.id_counter = 0;
		impl.items.clear();
		impl.requests.clear();
	}
};

//-----------------------------------------------------------------------------
template<typename T>
struct Entity
{
	int id;

	Entity() : id(-1) {}
	Entity(T* e) : id(e ? e->id : -1) {}
	Entity(T& e) : id(e.id) {}
	bool operator == (nullptr_t) const
	{
		return id == -1;
	}
	bool operator == (T* e) const
	{
		int id2 = e ? e->id : -1;
		return id == id2;
	}
	bool operator == (T& e) const
	{
		return id == e.id;
	}
	bool operator != (nullptr_t) const
	{
		return id != -1;
	}
	bool operator != (T* e) const
	{
		int id2 = e ? e->id : -1;
		return id != id2;
	}
	bool operator != (T& e) const
	{
		return id != e.id;
	}
	void operator = (nullptr_t)
	{
		id = -1;
	}
	void operator = (T* e)
	{
		if(e)
			id = e->id;
		else
			id = -1;
	}
	void operator = (T& e)
	{
		id = e.id;
	}
	operator bool() const
	{
		return id != -1;
	}
	operator T* ()
	{
		if(id == -1)
			return nullptr;
		T* e = EntityType<T>::GetById(id);
		if(e)
			return e;
		id = -1;
		return nullptr;
	}
};

template<typename T>
bool operator == (T* e, Entity<T>& ew)
{
	return ew == e;
}
template<typename T>
bool operator == (T& e, Entity<T>& ew)
{
	return ew == e;
}

template<typename T>
bool operator != (T* e, Entity<T>& ew)
{
	return ew != e;
}
template<typename T>
bool operator != (T& e, Entity<T>& ew)
{
	return ew != e;
}

//-----------------------------------------------------------------------------
template<typename T>
inline bool IsInside(const vector<Entity<T>>& v, const T* e)
{
	for(Entity<T> ew : v)
	{
		if(ew == e)
			return true;
	}
	return false;
}
