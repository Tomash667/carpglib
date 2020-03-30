#pragma once

//-----------------------------------------------------------------------------
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
