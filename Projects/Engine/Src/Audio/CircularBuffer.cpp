#include "PreCompiled.h"
#include "CircularBuffer.h"

void RS::CircularBuffer::Init(uint64 sampleRate)
{
	m_SampleRate = sampleRate;
	Size = sampleRate * MAX_CIRCULAR_BUFFER_SIZE * 2;
	pData = new float[Size];
	memset(pData, 0, Size * sizeof(float));
}

void RS::CircularBuffer::ClearAll()
{
	memset(pData, 0, Size * sizeof(float));
}

void RS::CircularBuffer::Clear()
{
	memset(pData, 0, (EndIndex + 1) * sizeof(float));
}

void RS::CircularBuffer::Destroy()
{
	if (pData)
	{
		delete pData;
		pData = nullptr;
	}
}

void RS::CircularBuffer::SetDelay(double seconds)
{
	EndIndex = glm::min((int64)(m_SampleRate * seconds * 2), (int64)Size - 1);
}

void RS::CircularBuffer::SetDelayInSamples(int64 samples)
{
	EndIndex = glm::min(samples * 2 - 1, (int64)Size - 1);
}

float RS::CircularBuffer::Get() const
{
    return pData[m_End];
}

void RS::CircularBuffer::Add(float v)
{
	pData[m_Start] = v;
	Next();
}

void RS::CircularBuffer::Next()
{
	auto NextV = [&](uint64 v)->uint64 {
		return (++v) % (EndIndex + 1);
	};

	m_Start = NextV(m_Start);
	m_End = NextV(m_End);
}
