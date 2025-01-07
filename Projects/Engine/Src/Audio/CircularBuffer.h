#pragma once

#include "Types.h"

#define MAX_CIRCULAR_BUFFER_SIZE 5 // In Seconds

namespace RS
{
	class CircularBuffer
	{
	public:
		uint64 Size = 0;
		uint64 EndIndex = 0;
		float* pData = nullptr; // This functions like a ring buffer.

	public:
		void Init(uint64 sampleRate);
		void ClearAll();
		void Clear();
		void Destroy();
		void SetDelay(double seconds);
		void SetDelayInSamples(int64 samples);

		/*
			Get oldest sample.
		*/
		float Get() const;

		/*
			Add new sample to the start.
		*/
		void Add(float v);

	private:
		void Next();

	private:
		uint64 m_Start		= 0;
		uint64 m_End		= 2;
		uint64 m_SampleRate = 0;
	};
}