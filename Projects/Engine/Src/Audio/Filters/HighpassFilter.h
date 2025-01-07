#pragma once

#include "Audio/Filters/Filter.h"
#include "Audio/CircularBuffer.h"

namespace RS
{
	class HighpassFilter : public Filter
	{
	public:
		void Init(uint64 sampleRate) override;
		void Destroy() override;
		void Begin() override;
		std::pair<float, float> Process(SoundData* pData, float left, float right) override;
		std::string GetName() const override;

		void SetCutoffFrequency(float cutoffFrequency);
		float GetCutoffFrequency() const;
		float GetSampleRate() const;

	private:
		CircularBuffer m_DelayBuffer;

		float m_CutoffFrequency = 20000.f;
		float m_SampleRate = 0.f;
	};
}