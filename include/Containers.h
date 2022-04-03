#pragma once

#include "CriticalSection.h"

//-----------------------------------------------------------------------------
// Containers helper functions
//-----------------------------------------------------------------------------
// Delete elemnt(s) from container
template<typename T>
inline void DeleteElements(vector<T>& v)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
		delete *it;
	v.clear();
}

template<typename T>
inline void DeleteElements(vector<T>* v)
{
	DeleteElements(*v);
}

template<typename T, typename Hash, typename Equals>
inline void DeleteElements(std::unordered_set<T, Hash, Equals>& v)
{
	for(auto e : v)
		delete e;
	v.clear();
}

template<typename Key, typename Value>
inline void DeleteElements(std::map<Key, Value>& v)
{
	for(auto& e : v)
		delete e.second;
	v.clear();
}

template<typename Key, typename Value, typename Hash>
inline void DeleteElements(std::unordered_map<Key, Value, Hash>& v)
{
	for(auto& e : v)
		delete e.second;
	v.clear();
}

template<typename T>
inline void DeleteElementsChecked(vector<T>& v)
{
	for(T& item : v)
	{
		assert(item);
		delete item;
	}
	v.clear();
}

template<typename T>
inline void DeleteElement(vector<T>& v, T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			delete *it;
			std::iter_swap(it, end - 1);
			v.pop_back();
			return;
		}
	}
	assert(0 && "Missing element to delete.");
}

template<typename T>
void DeleteElement(vector<T>* v, T& e)
{
	DeleteElement(*v, e);
}

template<typename T>
inline bool DeleteElementTry(vector<T>& v, T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			delete *it;
			std::iter_swap(it, end - 1);
			v.pop_back();
			return true;
		}
	}

	return false;
}

template<typename T>
inline void RemoveElement(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end - 1);
			v.pop_back();
			return;
		}
	}
	assert(0 && "Missing element to remove.");
}

template<typename T>
inline void RemoveElement(vector<T>* v, const T& e)
{
	RemoveElement(*v, e);
}

template<typename T>
inline void RemoveElementOrder(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			v.erase(it);
			return;
		}
	}
	assert(0 && "Missing element to remove.");
}

template<typename T>
inline void RemoveElementOrder(vector<T>* v, const T& e)
{
	RemoveElementOrder(*v, e);
}

template<typename T>
inline bool RemoveElementTry(vector<T>& v, const T& e)
{
	for(typename vector<T>::iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(e == *it)
		{
			std::iter_swap(it, end - 1);
			v.pop_back();
			return true;
		}
	}
	return false;
}

template<typename T>
inline bool RemoveElementTry(vector<T>* v, const T& e)
{
	return RemoveElementTry(*v, e);
}

template<typename T>
inline void RemoveElementIndex(vector<T>& v, int index)
{
	std::iter_swap(v.begin() + index, v.end() - 1);
	v.pop_back();
}

template<typename T>
inline void RemoveElementIndexOrder(vector<T>& v, int index)
{
	v.erase(v.begin() + index);
}

template<typename T>
inline bool is_null(const T ptr)
{
	return !ptr;
}

template<typename T>
inline void RemoveNullElements(vector<T>& v)
{
	auto it = std::remove_if(v.begin(), v.end(), is_null<T>);
	auto end = v.end();
	if(it != end)
		v.erase(it, end);
}

template<typename T>
inline void RemoveNullElements(vector<T>* v)
{
	assert(v);
	RemoveNullElements(*v);
}

template<typename T, typename T2>
inline void RemoveElements(vector<T>& v, T2 pred)
{
	auto it = std::remove_if(v.begin(), v.end(), pred);
	auto end = v.end();
	if(it != end)
		v.erase(it, end);
}

template<typename T, typename T2>
inline void RemoveElements(vector<T>* v, T2 pred)
{
	assert(v);
	RemoveElements(*v, pred);
}

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

template<typename T>
inline T& Add1(vector<T>* v)
{
	v->resize(v->size() + 1);
	return v->back();
}

template<typename T>
inline T& Add1(list<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

// Returns random item from vector
template<typename T>
inline T& RandomItem(vector<T>& v)
{
	return v[Rand() % v.size()];
}
template<typename T>
inline const T& RandomItem(const vector<T>& v)
{
	return v[Rand() % v.size()];
}

template<typename T>
inline T RandomItemPop(vector<T>& v)
{
	uint index = Rand() % v.size();
	T item = v[index];
	v.erase(v.begin() + index);
	return item;
}

template<class T>
inline T RandomItem(std::initializer_list<T> cont)
{
	int index = Rand() % cont.size();
	auto it = cont.begin();
	std::advance(it, index);
	return *it;
}

template<typename It>
inline void Shuffle(It begin, It end)
{
	std::shuffle(begin, end, internal::rng);
}

template<typename T>
inline bool IsInside(const vector<T>& v, const T& elem)
{
	for(typename vector<T>::const_iterator it = v.begin(), end = v.end(); it != end; ++it)
	{
		if(*it == elem)
			return true;
	}
	return false;
}

template<typename T>
inline bool IsInside(const vector<T>* v, const T& elem)
{
	return IsInside(*v, elem);
}

//-----------------------------------------------------------------------------
// Object pool pattern
//-----------------------------------------------------------------------------
#if defined(_DEBUG) && !defined(CORE_ONLY)
#	define CHECK_POOL_LEAKS
#endif
#ifdef CHECK_POOL_LEAKS
struct ObjectPoolLeakManager
{
	struct CallStackEntry;
	~ObjectPoolLeakManager();
	void Register(void* ptr);
	void Unregister(void* ptr);
	static ObjectPoolLeakManager instance;
private:
	vector<CallStackEntry*> call_stack_pool;
	std::unordered_map<void*, CallStackEntry*> call_stacks;
};
#endif

namespace internal
{
#define CALL_IF_EXISTS(Name) \
	template<typename T> \
	struct Has##Name##Method \
	{ \
		template<typename U, void(U::*)()> struct SFINAE {}; \
		template<typename U> static char Test(SFINAE<U, &U::Name>*); \
		template<typename U> static int Test(...); \
		static const bool Has = sizeof(Test<T>(0)) == sizeof(char); \
	}; \
	template<typename T> \
	void Call##Name(T* t, std::true_type) { t->Name(); } \
	template<typename T> \
	void Call##Name(T*, std::false_type) {} \
	template<typename T> \
	void Call##Name(T* t) { Call##Name(t, std::integral_constant<bool, Has##Name##Method<T>::Has>()); }

	CALL_IF_EXISTS(OnGet);
	CALL_IF_EXISTS(OnFree);
#undef CALL_IF_EXISTS
}

template<typename T>
struct ObjectPool
{
	ObjectPool() : destroyed(false)
	{
	}
	~ObjectPool()
	{
		Cleanup();
		destroyed = true;
	}

	T* Get()
	{
		T* e;
		if(pool.empty())
			e = new T;
		else
		{
			e = pool.back();
			pool.pop_back();
		}
		internal::CallOnGet(e);
#ifdef CHECK_POOL_LEAKS
		ObjectPoolLeakManager::instance.Register(e);
#endif
		return e;
	}

	T* Get(const T& val)
	{
		T* e = Get();
		*e = val;
		return e;
	}

	void Free(T* e)
	{
		assert(e && !destroyed);
#ifdef CHECK_POOL_LEAKS
		VerifyElement(e);
		ObjectPoolLeakManager::instance.Unregister(e);
#endif
		internal::CallOnFree(e);
		pool.push_back(e);
	}

	void Free(vector<T*>& elems)
	{
		assert(!destroyed);
		if(elems.empty())
			return;

#ifdef CHECK_POOL_LEAKS
		CheckDuplicates(elems);
#endif
		for(T* e : elems)
		{
			assert(e);
			internal::CallOnFree(e);
#ifdef CHECK_POOL_LEAKS
			VerifyElement(e);
			ObjectPoolLeakManager::instance.Unregister(e);
#endif
		}

		pool.insert(pool.end(), elems.begin(), elems.end());
		elems.clear();
	}

	void SafeFree(T* e)
	{
		if(!destroyed)
			Free(e);
		else
		{
			assert(e);
			internal::CallOnFree(e);
#ifdef CHECK_POOL_LEAKS
			ObjectPoolLeakManager::instance.Unregister(e);
#endif
			delete e;
		}
	}

	void SafeFree(vector<T*>& elems)
	{
#ifdef CHECK_POOL_LEAKS
		CheckDuplicates(elems);
#endif
		if(!destroyed)
		{
			for(T* e : elems)
			{
				if(e)
				{
					internal::CallOnFree(e);
#ifdef CHECK_POOL_LEAKS
					VerifyElement(e);
					ObjectPoolLeakManager::instance.Unregister(e);
#endif
					pool.push_back(e);
				}
			}
		}
		else
		{
			for(T* e : elems)
			{
				if(e)
				{
					internal::CallOnFree(e);
#ifdef CHECK_POOL_LEAKS
					ObjectPoolLeakManager::instance.Unregister(e);
#endif
					delete e;
				}
			}
		}
		elems.clear();
	}

	void Cleanup()
	{
		DeleteElements(pool);
	}

private:
	void VerifyElement(T* t)
	{
		for(T* e : pool)
			assert(t != e);
	}

	void CheckDuplicates(vector<T*>& elems)
	{
		for(uint i = 0, count = elems.size(); i < count; ++i)
		{
			T* t = elems[i];
			if(!t)
				continue;
			for(uint j = i + 1; j < count; ++j)
				assert(t != elems[j]);
		}
	}

	vector<T*> pool;
	bool destroyed;
};

template<typename T>
class ObjectPoolProxy
{
	friend struct ObjectPool<T>;
public:
	static T* Get() { return GetPool().Get(); }
	static void Free(T* t) { GetPool().Free(t); }
	static void Free(vector<T*>& ts) { GetPool().Free(ts); }
	static void SafeFree(T* t) { GetPool().SafeFree(t); }
	static void SafeFree(vector<T*>& ts) { GetPool().SafeFree(ts); }
	static void Cleanup() { GetPool().Cleanup(); }
	void Free() { Free((T*)this); }
	void SafeFree() { SafeFree((T*)this); }

protected:
	ObjectPoolProxy() {}
	~ObjectPoolProxy() {}
private:
	static ObjectPool<T>& GetPool() { static ObjectPool<T> pool; return pool; }
};

template<typename T>
class Pooled
{
public:
	Pooled() { ptr = ObjectPoolProxy<T>::Get(); }
	Pooled(T* ptr) : ptr(ptr) {}
	~Pooled() { if(ptr) ptr->Free(); }
	T* operator -> () { return ptr; }
	operator T& () { return *ptr; }
	T* Get() { return ptr; }
	T* Pin()
	{
		T* tmp = ptr;
		ptr = nullptr;
		return tmp;
	}

private:
	T* ptr;
};

namespace internal
{
	template<typename T>
	struct ObjectPoolAllocator : IAllocator<T>
	{
		static_assert(std::is_base_of<ObjectPoolProxy<T>, T>::value, "T must inherit from ObjectPoolProxy<T>");

		T* Create()
		{
			return T::Get();
		}

		void Destroy(T* item)
		{
			T::Free(item);
		}
	};

	template<typename T>
	struct ObjectPoolVectorAllocator : IVectorAllocator<T>
	{
		static_assert(std::is_base_of<ObjectPoolProxy<T>, T>::value, "T must inherit from ObjectPoolProxy<T>");

		void Destroy(vector<T*>& items)
		{
			T::Free(items);
		}
	};
}

template<typename T>
using ObjectPoolRef = Ptr<T, internal::ObjectPoolAllocator<T>>;

template<typename T>
using ObjectPoolVectorRef = VectorPtr<T, internal::ObjectPoolVectorAllocator<T>>;

// global pools
extern ObjectPool<string> StringPool;
extern ObjectPool<vector<void*>> VectorPool;

//-----------------------------------------------------------------------------
// Local string using string pool
//-----------------------------------------------------------------------------
struct LocalString
{
	LocalString()
	{
		s = StringPool.Get();
		s->clear();
	}

	LocalString(cstring str)
	{
		assert(str);
		s = StringPool.Get();
		*s = str;
	}

	LocalString(cstring str, cstring str_to)
	{
		s = StringPool.Get();
		uint len = str_to - str;
		s->resize(len);
		memcpy((char*)s->data(), str, len);
	}

	LocalString(const string& str)
	{
		s = StringPool.Get();
		*s = str;
	}

	~LocalString()
	{
		if(s)
			StringPool.Free(s);
	}

	operator cstring () const { return s->c_str(); }
	operator string& () { return *s; }
	operator const string& () const { return *s; }
	string& operator * () { return *s; }
	string* operator -> () { return s; }
	const string* operator -> () const { return s; }
	void operator = (cstring str) { *s = str; }
	void operator = (const string& str) { *s = str; }
	void operator += (cstring str) { *s += str; }
	void operator += (const string& str) { *s += str; }
	void operator += (char c) { *s += c; }
	bool operator == (cstring str) const { return *s == str; }
	bool operator == (const string& str) const { return *s == str; }
	bool operator == (const LocalString& str) const { return *s == *str.s; }
	bool operator != (cstring str) const { return *s != str; }
	bool operator != (const string& str) const { return *s != str; }
	bool operator != (const LocalString& str) const { return *s != *str.s; }

	char at_back(uint offset) const
	{
		assert(offset < s->size());
		return s->at(s->size() - 1 - offset);
	}

	char* data()
	{
		return const_cast<char*>(s->c_str());
	}

	void pop(uint count)
	{
		assert(s->size() > count);
		s->resize(s->size() - count);
	}

	string& get_ref()
	{
		return *s;
	}

	string* get_ptr()
	{
		return s;
	}

	bool empty() const
	{
		return s->empty();
	}

	cstring c_str() const
	{
		return s->c_str();
	}

	void clear()
	{
		s->clear();
	}

	uint length() const
	{
		return s->length();
	}

	string* Pin()
	{
		string* str = s;
		s = nullptr;
		return str;
	}

private:
	string* s;
};

//-----------------------------------------------------------------------------
// Local vector using pool, can only store pointers
//-----------------------------------------------------------------------------
template<typename T>
struct LocalVector
{
	static_assert(sizeof(T) == sizeof(void*), "LocalVector element must be pointer or have sizeof pointer.");

	typedef typename vector<T>::iterator Iterator;
	typedef typename vector<T>::const_iterator ConstIterator;

	LocalVector()
	{
		v = (vector<T>*)VectorPool.Get();
		v->clear();
	}
	explicit LocalVector(vector<T>& v2)
	{
		v = (vector<T>*)VectorPool.Get();
		*v = v2;
	}
	~LocalVector()
	{
		VectorPool.Free((vector<void*>*)v);
	}

	operator vector<T>& () { return *v; }
	operator vector<T>* () { return v; }
	vector<T>* operator -> () { return v; }
	const vector<T>* operator -> () const { return v; }
	T& operator [] (int n) { return v->at(n); }
	const T& operator [] (int n) const { return v->at(n); }

	vector<T>& Get() { return *v; }
	T& RandomItem() { return v->at(Rand() % v->size()); }
	void Shuffle() { ::Shuffle(v->begin(), v->end()); }

	Iterator begin() { return v->begin(); }
	ConstIterator begin() const { return v->begin(); }
	void clear() { v->clear(); }
	bool empty() const { return v->empty(); }
	Iterator end() { return v->end(); }
	ConstIterator end() const { return v->end(); }
	void push_back(T e) { v->push_back(e); }
	uint size() const { return v->size(); }

private:
	vector<T>* v;
};

//-----------------------------------------------------------------------------
template<typename T>
struct rvector
{
	struct iter
	{
		typedef typename vector<T*>::iterator InnerIt;

		bool operator != (const iter& it2)
		{
			return it != it2.it;
		}
		iter& operator ++ ()
		{
			++it;
			return *this;
		}
		T& operator * ()
		{
			return **it;
		}

		InnerIt it;
	};

	struct const_iter
	{
		typedef typename vector<T*>::const_iterator InnerIt;

		bool operator != (const const_iter& it2)
		{
			return it != it2.it;
		}
		const_iter& operator ++ ()
		{
			++it;
			return *this;
		}
		const T& operator * ()
		{
			return **it;
		}

		InnerIt it;
	};

	T& operator [](uint index) { return *ptrs[index]; }

	iter begin() { return iter{ ptrs.begin() }; }
	const_iter begin() const { return const_iter{ ptrs.begin() }; }
	iter end() { return iter{ ptrs.end() }; }
	const_iter end() const { return const_iter{ ptrs.end() }; }
	uint size() const { return ptrs.size(); }
	void push_back(T* ptr)
	{
		assert(ptr);
		ptrs.push_back(ptr);
	}
	void erase(int index) { ptrs.erase(ptrs.begin() + index); }
	void erase(iter it) { ptrs.erase(it.it); }
	void erase(iter start, iter end) { ptrs.erase(start.it, end.it); }
	void clear() { ptrs.clear(); }
	T& front() { return *ptrs.front(); }
	const T& front() const { return *ptrs.front(); }

	vector<T*> ptrs;
};

template<typename T>
void DeleteElements(rvector<T>& v)
{
	DeleteElements(v.ptrs);
}

//-----------------------------------------------------------------------------
// Loop over items and erase elements that returned true
template<typename T, typename Action>
inline void LoopAndRemove(vector<T>& items, Action action)
{
	items.erase(std::remove_if(items.begin(), items.end(), action), items.end());
}
template<typename T, typename Action>
inline void LoopAndRemove(rvector<T>& items, Action action)
{
	items.ptrs.erase(std::remove_if(items.ptrs.begin(), items.ptrs.end(), [&](T* ptr) { return action(*ptr); }), items.ptrs.end());
}
template<typename Key, typename Value, typename Action>
inline void LoopAndRemove(std::map<Key, Value>& items, Action action)
{
	for(auto it = items.begin(), end = items.end(); it != end;)
	{
		if(action(it->second))
		{
			it = items.erase(it);
			end = items.end();
		}
		else
			++it;
	}
}

//-----------------------------------------------------------------------------
// Loop over items and delete elements that returned true
template<typename T, typename Pred>
inline void DeleteElements(vector<T>& items, Pred pred)
{
	items.erase(std::remove_if(items.begin(), items.end(), [&](T item)
		{
			if(pred(item))
			{
				delete item;
				return true;
			}
			return false;
		}), items.end());
}

//-----------------------------------------------------------------------------
// Return random weighted item
template<typename T>
struct WeightPair
{
	T item;
	int weight;

	WeightPair(T& item, int weight) : item(item), weight(weight) {}
};

template<typename T>
inline T& RandomItemWeight(vector<WeightPair<T>>& items, int max_weight)
{
	if(items.size() == (uint)max_weight)
		return RandomItem(items).item;
	int a = Rand() % max_weight, b = 0;
	for(auto& item : items)
	{
		b += item.weight;
		if(a < b)
			return item.item;
	}
	// if it gets here max_count is wrong, return random item
	return RandomItem(items).item;
}

template<typename T, typename GetWeight, typename GetItem>
inline auto RandomItemWeight(const vector<T>& items, int max_weight, GetWeight get_weight, GetItem get_item)
{
	int a = Rand() % max_weight, b = 0;
	for(auto& item : items)
	{
		b += get_weight(item);
		if(a < b)
			return get_item(item);
	}
	// if it gets here max_count is wrong, return random item
	return get_item(items[Rand() % items.size()]);
}

template<typename T>
struct WeightContainer
{
	WeightContainer() : total(0) {}
	void Add(T& item, int weight)
	{
		assert(weight >= 1);
		items.push_back(WeightPair<T>(item, weight));
		total += weight;
	}
	T& GetRandom()
	{
		return RandomItemWeight(items, total);
	}
private:
	vector<WeightPair<T>> items;
	int total;
};

//-----------------------------------------------------------------------------
// Like LocalVector but can store any data
extern ObjectPool<vector<byte>> BufPool;

template<typename T>
class LocalVector3
{
public:
	struct iterator
	{
		typedef std::random_access_iterator_tag iterator_category;
		typedef T value_type;
		typedef std::ptrdiff_t difference_type;
		typedef T* pointer;
		typedef T& reference;

		friend class LocalVector3;

		T& operator * () const
		{
			assert(offset < (int)v->size());
			return v->at(offset);
		}
		T& operator [](difference_type n) const
		{
			const int pos = offset + n;
			assert(pos >= 0 && pos < (int)v->size());
			return v->at(pos);
		}
		bool operator == (const iterator& it) const
		{
			assert(it.v == v);
			return offset == it.offset;
		}
		bool operator != (const iterator& it) const
		{
			assert(it.v == v);
			return offset != it.offset;
		}
		bool operator > (const iterator& it) const
		{
			assert(it.v == v);
			return offset > it.offset;
		}
		bool operator >= (const iterator& it) const
		{
			assert(it.v == v);
			return offset >= it.offset;
		}
		bool operator < (const iterator& it) const
		{
			assert(it.v == v);
			return offset < it.offset;
		}
		bool operator <= (const iterator& it) const
		{
			assert(it.v == v);
			return offset <= it.offset;
		}
		iterator& operator ++ ()
		{
			assert(offset != (int)v->size());
			++offset;
			return *this;
		}
		iterator operator ++ (int)
		{
			assert(offset != (int)v->size());
			iterator it(v, offset);
			++offset;
			return it;
		}
		iterator& operator -- ()
		{
			assert(offset != 0);
			--offset;
			return *this;
		}
		iterator operator -- (int)
		{
			assert(offset != 0);
			iterator it(v, offset);
			--offset;
			return it;
		}
		iterator& operator += (difference_type n)
		{
			offset += n;
			assert(offset >= 0 && offset <= (int)v->size());
			return *this;
		}
		iterator& operator -= (difference_type n)
		{
			offset -= n;
			assert(offset >= 0 && offset <= (int)v->size());
			return *this;
		}
		iterator operator + (difference_type n) const
		{
			return iterator(v, offset + n);
		}
		friend iterator operator + (difference_type n, const iterator& it)
		{
			return iterator(v, offset + n);
		}
		iterator operator - (difference_type n) const
		{
			return iterator(v, offset - n);
		}
		difference_type operator - (const iterator& it) const
		{
			assert(it.v == v);
			return offset - it.offset;
		}

	private:
		iterator(LocalVector3* v, int offset) : v(v), offset(offset)
		{
			assert(v && offset >= 0 && offset <= (int)v->size());
		}

		LocalVector3* v;
		int offset;
	};

	LocalVector3()
	{
		buf = BufPool.Get();
		buf->clear();
	}

	~LocalVector3()
	{
		BufPool.Free(buf);
	}

	T& at(uint offset)
	{
		assert(offset < size());
		return ((T*)buf->data())[offset];
	}

	T& back()
	{
		assert(!empty());
		return ((T*)buf->data())[size() - 1];
	}

	iterator begin()
	{
		return iterator(this, 0);
	}

	bool empty() const
	{
		return buf->empty();
	}

	iterator end()
	{
		return iterator(this, size());
	}

	void push_back(const T& item)
	{
		uint s = buf->size();
		buf->resize(buf->size() + sizeof(T));
		memcpy(buf->data() + s, &item, sizeof(T));
	}

	uint size() const
	{
		return buf->size() / sizeof(T);
	}

	template<typename Pred>
	void sort(Pred pred)
	{
		std::sort((T*)buf->data(), (T*)(buf->data() + buf->size()), pred);
	}

private:
	vector<byte>* buf;
};

//-----------------------------------------------------------------------------
// Find index of item
template<typename T>
inline int GetIndex(const vector<T>& items, const T& item)
{
	int index = 0;
	for(const T& it : items)
	{
		if(it == item)
			return index;
		++index;
	}
	return -1;
}

template<typename T, typename Pred>
inline int GetIndex(const vector<T>& items, Pred pred)
{
	int index = 0;
	for(const T& it : items)
	{
		if(pred(it))
			return index;
		++index;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Vector using critical section protection
template<typename T>
class SafeVector
{
public:
	SafeVector()
	{
		cs.Create();
	}

	~SafeVector()
	{
		cs.Free();
	}

	inline void Push(T& e)
	{
		StartCriticalSection section(cs);
		v.push_back(e);
	}

	inline T Pop()
	{
		StartCriticalSection section(cs);
		T e = v.back();
		v.pop_back();
		return e;
	}

	inline bool Any()
	{
		return !v.empty();
	}

private:
	vector<T> v;
	CriticalSection cs;
};

namespace helper
{
	template<typename T>
	struct DerefEnumerator
	{
		struct Iterator
		{
			typedef typename vector<T*>::iterator It;

			Iterator(It it) : it(it)
			{
			}

			bool operator != (const Iterator& i) const
			{
				return it != i.it;
			}

			void operator ++ ()
			{
				++it;
			}

			T& operator * ()
			{
				return **it;
			}

			It it;
		};

		DerefEnumerator(vector<T*>& v) : v(v)
		{
		}

		Iterator begin()
		{
			return Iterator(v.begin());
		}

		Iterator end()
		{
			return Iterator(v.end());
		}

		vector<T*>& v;
	};
}

template<typename T>
helper::DerefEnumerator<T> Deref(vector<T*>& v)
{
	return helper::DerefEnumerator<T>(v);
}

//-----------------------------------------------------------------------------
namespace internal
{
	template<typename T>
	struct Hash
	{
		size_t operator () (const T* obj) const
		{
			std::hash<string> hasher;
			return hasher(obj->id);
		}
	};

	template<typename T>
	struct Equals
	{
		bool operator () (const T* obj1, const T* obj2) const
		{
			return obj1->id == obj2->id;
		}
	};
}

template<typename T>
using SetContainer = std::unordered_set<T*, internal::Hash<T>, internal::Equals<T>>;

//-----------------------------------------------------------------------------
template<typename T>
struct PointerVector
{
	typedef typename vector<T*> Vector;
	typedef typename vector<T*>::iterator Iter;
	typedef typename vector<T*>::const_iterator ConstIter;

	struct Iterator
	{
		Iter it;

		Iterator(Iter it) : it(it) {}
		bool operator != (const Iterator& i)
		{
			return it != i.it;
		}
		void operator ++ ()
		{
			++it;
		}
		T& operator * ()
		{
			return **it;
		}
		T* operator -> ()
		{
			return *it;
		}
		Iterator operator + (int offset) const
		{
			return Iterator(it + offset);
		}
		Iterator operator - (int offset) const
		{
			return Iterator(it - offset);
		}
		T* ptr()
		{
			return *it;
		}
	};

	struct ConstIterator
	{
		ConstIter it;

		ConstIterator(ConstIter it) : it(it) {}
		bool operator != (const ConstIterator& i)
		{
			return it != i.it;
		}
		void operator ++ ()
		{
			++it;
		}
		const T& operator * ()
		{
			return **it;
		}
		const T* operator -> () const
		{
			return *it;
		}
		ConstIterator operator + (int offset) const
		{
			return Iterator(it + offset);
		}
		ConstIterator operator - (int offset) const
		{
			return Iterator(it - offset);
		}
		const T* ptr() const
		{
			return *it;
		}
	};

	Iterator begin()
	{
		return Iterator(v.begin());
	}
	ConstIterator begin() const
	{
		return ConstIterator(v.begin());
	}
	Iterator end()
	{
		return Iterator(v.end());
	}
	ConstIterator end() const
	{
		return ConstIterator(v.end());
	}

	bool empty() const
	{
		return v.empty();
	}
	uint size() const
	{
		return v.size();
	}
	T& operator [] (uint index)
	{
		return *v[index];
	}
	T* ptr(uint index)
	{
		return v[index];
	}
	void reserve(uint size)
	{
		v.reserve(size);
	}
	void resize(uint size)
	{
		v.resize(size);
	}
	void push_back(T* e)
	{
		v.push_back(e);
	}
	Iterator erase(Iterator it)
	{
		return Iterator(v.erase(it.it));
	}
	ConstIterator erase(ConstIterator it) const
	{
		return ConstIterator(v.erase(it.it));
	}
	void pop_back()
	{
		v.pop_back();
	}
	void clear()
	{
		v.clear();
	}

	Vector v;
};

//-----------------------------------------------------------------------------
// Buffer - used by MemoryStream
class Buffer : public ObjectPoolProxy<Buffer>
{
public:
	void Append(void* ptr, uint size)
	{
		if(size == 0)
			return;
		uint offset = data.size();
		data.resize(offset + size);
		memcpy(data.data() + offset, ptr, size);
	}
	cstring AsString()
	{
		if(data.size() != 0u)
		{
			if(data.back() == 0)
				return (cstring)data.data();
		}
		data.push_back(0);
		return (cstring)data.data();
	}
	void* At(uint offset) { return data.data() + offset; }
	void Clear() { data.clear(); }
	void* Data() { return data.data(); }
	// decompress buffer to new buffer and return it, old one is freed
#ifndef CORE_ONLY
	Buffer* Decompress(uint real_size);
#endif
	void Resize(uint size) { data.resize(size); }
	uint Size() const { return data.size(); }

private:
	vector<byte> data;
};

//-----------------------------------------------------------------------------
struct BufferHandle
{
	BufferHandle(Buffer* buf) : buf(buf) {}
	~BufferHandle()
	{
		if(buf)
			buf->Free();
	}

	operator bool() const
	{
		return buf != nullptr;
	}

	Buffer* operator -> ()
	{
		return buf;
	}

	Buffer* Pin()
	{
		Buffer* b = buf;
		buf = nullptr;
		return b;
	}

private:
	Buffer* buf;
};

//-----------------------------------------------------------------------------
struct Buf
{
	Buf()
	{
		buf = BufPool.Get();
	}
	~Buf()
	{
		BufPool.Free(buf);
	}
	template<typename T>
	T* Get(uint size)
	{
		buf->resize(size);
		return reinterpret_cast<T*>(buf->data());
	}
	void* Get()
	{
		return buf->data();
	}

private:
	vector<byte>* buf;
};

//-----------------------------------------------------------------------------
template<typename T>
struct RemoveRandomPred
{
	int chance, a, b;

	RemoveRandomPred(int chance, int a, int b) : chance(chance), a(a), b(b)
	{
	}

	bool operator () (const T&)
	{
		return Random(a, b) < chance;
	}
};
