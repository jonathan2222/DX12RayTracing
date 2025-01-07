#include "PreCompiled.h"
#include "EchoFilter.h"

void RS::EchoFilter::Init(uint64 sampleRate)
{
    m_DelayBuffer.Init(sampleRate);
}

void RS::EchoFilter::Destroy()
{
}

void RS::EchoFilter::Begin()
{
    m_DelayBuffer.SetDelay(m_Delay);
}

std::pair<float, float> RS::EchoFilter::Process(SoundData* pData, float left, float right)
{
	float oldLeftSampel = m_DelayBuffer.Get();
	float newLeftInput = left + m_Gain * oldLeftSampel;
	m_DelayBuffer.Add(newLeftInput);

	float oldRightSampel = m_DelayBuffer.Get();
	float newRightInput = right + m_Gain * oldRightSampel;
	m_DelayBuffer.Add(newRightInput);

	return std::pair<float, float>(oldLeftSampel, oldRightSampel);
}

std::string RS::EchoFilter::GetName() const
{
    return "Echo Filter";
}

void RS::EchoFilter::SetDelay(float delay)
{
	if (glm::abs(m_Delay - delay) > 0.0001f)
		m_DelayBuffer.Clear();
	m_Delay = delay;
}

void RS::EchoFilter::SetGain(float gain)
{
	m_Gain = gain;
}

float RS::EchoFilter::GetDelay() const
{
    return m_Delay;
}

float RS::EchoFilter::GetGain() const
{
    return m_Gain;
}
