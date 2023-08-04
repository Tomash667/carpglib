#pragma once

//-----------------------------------------------------------------------------
class Timer
{
public:
	explicit Timer(bool start = true);

	void Start();
	float Tick();
	void Reset();

	void GetTime(int64& time) const { time = lastTime; }
	double GetTicksPerSec() const { return ticksPerSec; }
	bool IsStarted() const { return started; }

private:
	double ticksPerSec;
	int64 lastTime;
	bool started;
};
