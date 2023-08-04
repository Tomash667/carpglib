#include "Pch.h"
#include "Timer.h"
#include "WindowsIncludes.h"

//=================================================================================================
Timer::Timer(bool start) : started(false)
{
	if(start)
		Start();
}

//=================================================================================================
void Timer::Start()
{
	LARGE_INTEGER qwTicksPerSec = { 0 };
	QueryPerformanceFrequency(&qwTicksPerSec);

	LARGE_INTEGER qwTime;
	ticksPerSec = (double)qwTicksPerSec.QuadPart;
	QueryPerformanceCounter(&qwTime);
	lastTime = qwTime.QuadPart;

	started = true;
}

//=================================================================================================
float Timer::Tick()
{
	assert(started);

	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	float delta = (float)((double)(qwTime.QuadPart - lastTime) / ticksPerSec);
	lastTime = qwTime.QuadPart;

	return delta < 0 ? 0 : delta;
}

//=================================================================================================
void Timer::Reset()
{
	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	lastTime = qwTime.QuadPart;
}
