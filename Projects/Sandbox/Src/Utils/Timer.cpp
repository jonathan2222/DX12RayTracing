#include "PreCompiled.h"
#include "Timer.h"

using namespace RS;

Timer::Timer()
{
	Start();
}

Timer::~Timer()
{
}

void Timer::Start()
{
	m_StartTime = std::chrono::high_resolution_clock::now();
}

TimeStamp Timer::Stop()
{
	m_EndTime = std::chrono::high_resolution_clock::now();
	m_TimeStamp.m_DT = std::chrono::duration<double, std::milli>(m_EndTime - m_StartTime).count();
	return m_TimeStamp;
}

TimeStamp Timer::CalcDelta()
{
	std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::high_resolution_clock::now();
	m_TimeStamp.m_DT = std::chrono::duration<double, std::milli>(time - m_StartTime).count();
	m_StartTime = time;
	return m_TimeStamp;
}

float TimeStamp::GetDeltaTimeSec() const
{
	return (float)(m_DT / 1000.0);
}

float TimeStamp::GetDeltaTimeMS() const
{
	return (float)m_DT;
}
