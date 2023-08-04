#include "Pch.h"
#ifdef CHECK_POOL_LEAKS
#	include "WindowsIncludes.h"
#	define IN
#	define OUT
#	pragma warning(push)
#	pragma warning(disable:4091)
#	include <Dbghelp.h>
#	pragma warning(pop)
#endif
#undef FAR
#include <zlib.h>

ObjectPool<string> StringPool;
ObjectPool<vector<void*>> VectorPool;
ObjectPool<vector<byte>> BufPool;

#ifdef CHECK_POOL_LEAKS

ObjectPoolLeakManager ObjectPoolLeakManager::instance;

struct ObjectPoolLeakManager::CallStackEntry
{
	static const uint MAX_FRAMES = 32;
	void* frames[MAX_FRAMES];
};

ObjectPoolLeakManager::~ObjectPoolLeakManager()
{
	if(!callStacks.empty())
	{
		OutputDebugString(Format("!!!!!!!!!!!!!!!!!!!!!!!!!!\nObjectPool leaks detected (%u):\n", callStacks.size()));

		HANDLE handle = GetCurrentProcess();
		SymInitialize(handle, nullptr, true);
		byte symbolData[sizeof(SYMBOL_INFO) + 1000] = { 0 };
		SYMBOL_INFO& symbol = *(SYMBOL_INFO*)&symbolData[0];
		symbol.SizeOfStruct = sizeof(SYMBOL_INFO);
		symbol.MaxNameLen = 1000;
		DWORD64 displacement;
		IMAGEHLP_LINE64 line;

		uint index = 0;
		for(auto& pcs : callStacks)
		{
			OutputDebugString(Format("[%u] Address: %p Call stack:\n", index, pcs.first));
			CallStackEntry& cs = *pcs.second;
			for(uint i = 0; i < CallStackEntry::MAX_FRAMES; ++i)
			{
				SymFromAddr(handle, (DWORD64)(uint)cs.frames[i], &displacement, &symbol);
				DWORD disp;
				SymGetLineFromAddr64(handle, (DWORD64)(uint)cs.frames[i], &disp, &line);
				OutputDebugString(Format("\t%s (%d): %s\n", line.FileName, line.LineNumber, symbol.Name));
				if(cs.frames[i] == nullptr)
					break;
			}
			delete pcs.second;
			++index;
			OutputDebugString("\n\n");
		}
	}

	DeleteElements(callStackPool);
}

void ObjectPoolLeakManager::Register(void* ptr)
{
	assert(ptr && ptr != (void*)0xCDCDCDCD);
	assert(callStacks.find(ptr) == callStacks.end());

	CallStackEntry* cs;
	if(callStackPool.empty())
		cs = new CallStackEntry;
	else
	{
		cs = callStackPool.back();
		callStackPool.pop_back();
	}

	memset(cs, 0, sizeof(CallStackEntry));

	RtlCaptureStackBackTrace(1, CallStackEntry::MAX_FRAMES, cs->frames, nullptr);

	callStacks[ptr] = cs;
}

void ObjectPoolLeakManager::Unregister(void* ptr)
{
	assert(ptr && ptr != (void*)0xCDCDCDCD);

	auto it = callStacks.find(ptr);
	assert(it != callStacks.end());
	callStackPool.push_back(it->second);
	callStacks.erase(it);
}

#endif

//=================================================================================================
Buffer* Buffer::Compress()
{
	uint safeSize = compressBound(Size());
	Buffer* buf = Buffer::Get();
	buf->Resize(safeSize);
	uint realSize = safeSize;
	compress(static_cast<Bytef*>(buf->Data()), (uLongf*)&realSize, static_cast<const Bytef*>(Data()), Size());
	buf->Resize(realSize);
	Free();
	return buf;
}

//=================================================================================================
Buffer* Buffer::TryCompress()
{
	uint safeSize = compressBound(Size());
	Buffer* buf = Buffer::Get();
	buf->Resize(safeSize);
	uint realSize = safeSize;
	compress(static_cast<Bytef*>(buf->Data()), (uLongf*)&realSize, static_cast<const Bytef*>(Data()), Size());
	if(realSize < Size())
	{
		Free();
		buf->Resize(realSize);
		return buf;
	}
	else
	{
		buf->Free();
		return this;
	}
}

//=================================================================================================
Buffer* Buffer::Decompress(uint realSize)
{
	Buffer* buf = Buffer::Get();
	buf->Resize(realSize);
	uLong size = realSize;
	uncompress(static_cast<Bytef*>(buf->Data()), &size, static_cast<const Bytef*>(Data()), Size());
	Free();
	return buf;
}
