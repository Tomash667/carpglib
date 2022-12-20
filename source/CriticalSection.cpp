#include "Pch.h"
#include "WindowsIncludes.h"

void CriticalSection::Create(uint spinCount)
{
	if(!handle)
	{
		CRITICAL_SECTION* cs = new CRITICAL_SECTION;
		InitializeCriticalSectionAndSpinCount(cs, spinCount);
		handle = cs;
	}
}

void CriticalSection::Free()
{
	if(handle)
	{
		CRITICAL_SECTION* cs = (CRITICAL_SECTION*)handle;
		DeleteCriticalSection(cs);
		delete cs;
		handle = nullptr;
	}
}

void CriticalSection::Enter()
{
	assert(handle);
	EnterCriticalSection((CRITICAL_SECTION*)handle);
}

void CriticalSection::Leave()
{
	assert(handle);
	LeaveCriticalSection((CRITICAL_SECTION*)handle);
}
