#include "PreCompiled.h"
#include "HighpassFilter.h"

#include <glm/gtc/constants.hpp>

void RS::HighpassFilter::Init(uint64 sampleRate)
{
    m_SampleRate = (float)sampleRate;
    m_CutoffFrequency = m_SampleRate * 0.25f;
    m_DelayBuffer.Init(sampleRate);
}

void RS::HighpassFilter::Destroy()
{
}

void RS::HighpassFilter::Begin()
{
    m_DelayBuffer.SetDelayInSamples(1); // One sample per ear
}

std::pair<float, float> RS::HighpassFilter::Process(SoundData* pData, float left, float right)
{
	// Highpass calculations
	constexpr float PI = glm::pi<float>();
	const float fs = m_SampleRate;
	float product = 2.f * PI * m_CutoffFrequency / fs;
	float tmp = (2.f - glm::cos(product));
	float b = 2.f - glm::cos(product) - glm::sqrt(tmp * tmp - 1.f);
	float a = 1.f - b;

	// Apply y(n) = b*x(n) - a*y(n-1)
	float oldLeftSampel = m_DelayBuffer.Get();	// get y(n-1)
	float newLeft = a * left - b * oldLeftSampel;
	m_DelayBuffer.Add(newLeft);

	float oldRightSampel = m_DelayBuffer.Get(); // get y(n-1)
	float newRight = a * right - b * oldRightSampel;
	m_DelayBuffer.Add(newRight);

	return std::pair<float, float>(newLeft, newRight);
}

std::string RS::HighpassFilter::GetName() const
{
    return "Highpass Filter";
}

void RS::HighpassFilter::SetCutoffFrequency(float cutoffFrequency)
{
	if (glm::abs(m_CutoffFrequency - cutoffFrequency) > 0.0001f)
		m_DelayBuffer.Clear();
	m_CutoffFrequency = cutoffFrequency;
}

float RS::HighpassFilter::GetCutoffFrequency() const
{
    return m_CutoffFrequency;
}

float RS::HighpassFilter::GetSampleRate() const
{
    return m_SampleRate;
}
