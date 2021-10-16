#pragma once

#include <chrono>

namespace RS
{
	class TimeStamp
	{
	public:
		float GetDeltaTimeSec() const;
		float GetDeltaTimeMS() const;

	private:
		double m_DT = 0.0;
		friend class Timer;
	};

	class Timer
	{
	public:
		Timer();
		~Timer();

		void Start();
		TimeStamp Stop();

		/*
		* This works like a stop and start. It will restart the timer and return the time since previous start/restart.
		*/
		TimeStamp CalcDelta();

	private:
		std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
		std::chrono::time_point<std::chrono::steady_clock> m_EndTime;
		TimeStamp m_TimeStamp;
	};
}