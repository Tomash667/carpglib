#pragma once

//-----------------------------------------------------------------------------
#include "Render.h"
#include "WindowsIncludes.h"
#define far
#include <d3d11_1.h>
#undef DrawText
#undef CreateFont
#undef GetCharWidth

//-----------------------------------------------------------------------------
#ifdef _DEBUG
#	define V(x) assert(SUCCEEDED(x))
#else
#	define V(x) (x)
#endif

//-----------------------------------------------------------------------------
template<typename T>
inline void SafeRelease(T& x)
{
	if(x)
	{
		x->Release();
		x = nullptr;
	}
}

//-----------------------------------------------------------------------------
namespace internal
{
	template<typename T>
	struct ComAllocator : IAllocator<T>
	{
		T* Create() { return nullptr; }
		void Destroy(T* item) { SafeRelease(item); }
	};
}

template<typename T>
using CPtr = Ptr<T, internal::ComAllocator<T>>;

//-----------------------------------------------------------------------------
struct ResourceLock
{
	ResourceLock(ID3D11Resource* res, D3D11_MAP mode = D3D11_MAP_WRITE)
	{
		this->res = res;
		D3D11_MAPPED_SUBRESOURCE sub;
		V(app::render->GetDeviceContext()->Map(res, 0, mode, 0, &sub));
		data = sub.pData;
	}
	~ResourceLock()
	{
		app::render->GetDeviceContext()->Unmap(res, 0);
	}
	template<typename T>
	T* Get()
	{
		return reinterpret_cast<T*>(data);
	}
	void* Get()
	{
		return data;
	}

private:
	ID3D11Resource* res;
	void* data;
};
