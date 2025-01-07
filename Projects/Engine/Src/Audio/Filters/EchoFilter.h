#pragma once

#include "Audio/Filters/Filter.h"
#include "Audio/CircularBuffer.h"

namespace RS
{
	class EchoFilter : public Filter
	{
	public:
		void Init(uint64 sampleRate) override;
		void Destroy() override;
		void Begin() override;
		std::pair<float, float> Process(SoundData* pData, float left, float right) override;
		std::string GetName() const override;

		void SetDelay(float delay);
		void SetGain(float gain);

		float GetDelay() const;
		float GetGain() const;

	private:
		CircularBuffer m_DelayBuffer;

		float m_Delay = 0.5f;
		float m_Gain = 0.5f;
	};
}